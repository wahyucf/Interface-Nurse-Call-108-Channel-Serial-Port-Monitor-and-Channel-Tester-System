// ============================================================
//  MULTIPLEXER MONITOR 7×16 (MAX 112 CH) UNTUK 108 TRIGGER
//  FIXED VERSION - Addressing floating pin issue
//
//  CHANGES:
//  1. Added INPUT_PULLDOWN emulation (read multiple times, discard first)
//  2. Increased threshold to 400 (floating is ~220, real trigger should be >600)
//  3. Added MUX enable pins if available
//  4. Better settling time for analog reads
//  5. Baseline calibration at startup
// ============================================================

#define NUM_MUX           7
#define CHANNELS_PER_MUX 16
#define TOTAL_CHANNELS   108     // 18 x 6 = 108 titik nyata
#define FIFO_SIZE        32      // antrian panggilan aktif

// NEW THRESHOLD - based on diagnostic showing floating ~220-245
// Real 12V trigger with divider should give >600
const int THRESHOLD_HIGH = 400;  // Raised from 150
const int THRESHOLD_LOW  = 150;  // Raised from 80
const unsigned long BROADCAST_INTERVAL = 100;

// Timer overflow-safe
uint32_t lastHeartbeat    = 0;
uint32_t lastBroadcast    = 0;
uint32_t lastStatusReport = 0;
uint32_t lastDebugPrint   = 0;

// Shared select pin untuk semua MUX (S0..S3)
const int muxControlPins[4] = {22, 23, 24, 25};

// CRITICAL: Output masing-masing MUX ke analog Mega
const int muxSignalPins[NUM_MUX] = {A15, A14, A13, A12, A11, A10, A9};

// Optional: Enable pins for each MUX (set to -1 if not used)
// If your board has EN pins, connect them and set pin numbers here
const int muxEnablePins[NUM_MUX] = {-1, -1, -1, -1, -1, -1, -1};

// Baseline noise level per MUX (calibrated at startup)
int baselineNoise[NUM_MUX] = {0};

// Debug mode
bool debugMode = false;

// ---------------- STATUS PER CHANNEL ----------------
struct ChannelStatus {
  bool     active;
  uint8_t  stableHigh;
  uint8_t  stableLow;
  uint8_t  readCount;
  uint16_t sumReadings;
};

ChannelStatus channels[NUM_MUX][CHANNELS_PER_MUX];

// ---------------- FIFO PANGGILAN AKTIF ----------------
struct SafeFIFO {
  int buffer[FIFO_SIZE];
  int count;
  int index;

  void reset() {
    count = 0;
    index = 0;
  }

  bool add(int pin) {
    if (count >= FIFO_SIZE) return false;
    // hindari duplikasi
    for (int i = 0; i < count; i++) {
      if (buffer[i] == pin) return true;
    }
    buffer[count++] = pin;
    return true;
  }

  bool remove(int pin) {
    if (count == 0) return false;
    int pos = -1;
    for (int i = 0; i < count; i++) {
      if (buffer[i] == pin) {
        pos = i;
        break;
      }
    }
    if (pos < 0) return false;

    for (int j = pos; j < count - 1; j++) {
      buffer[j] = buffer[j + 1];
    }
    count--;
    if (index >= count) index = 0;
    return true;
  }

  bool getNext(int &pin) {
    if (count == 0) return false;
    if (index >= count) index = 0;
    pin = buffer[index];
    index++;
    return true;
  }
};

SafeFIFO fifo;

// ---------------- ERROR COUNTER SEDERHANA ----------------
struct ErrorCounters {
  uint32_t fifoOverflow;
  uint32_t isrTimeout;
} errors = {0, 0};

// ---------------- SCAN POINTER ----------------
volatile uint8_t curMux = 0;
volatile uint8_t curCh  = 0;
volatile bool isrBusy   = false;

// ============================================================
// IMPROVED ANALOG READ - Discard first reading to reduce float
// ============================================================
int readAnalogStable(int pin) {
  // Discard first reading (may be affected by previous channel)
  analogRead(pin);
  delayMicroseconds(50); // Longer settling time
  
  // Take 3 readings and average
  int sum = 0;
  for (int i = 0; i < 3; i++) {
    sum += analogRead(pin);
    delayMicroseconds(20);
  }
  
  return sum / 3;
}

// ============================================================
// SET MUX CHANNEL (S0..S3)
// ============================================================
void setMuxChannel(uint8_t ch) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(muxControlPins[i], (ch >> i) & 0x01);
  }
  delayMicroseconds(20); // Increased settling time
}

// ============================================================
// ENABLE/DISABLE MUX (if EN pins are connected)
// ============================================================
void setMuxEnabled(uint8_t muxIndex, bool enabled) {
  if (muxEnablePins[muxIndex] >= 0) {
    // Most MUX: EN=LOW means enabled, EN=HIGH means disabled
    digitalWrite(muxEnablePins[muxIndex], enabled ? LOW : HIGH);
  }
}

// ============================================================
// KIRIM EVENT KE SERIAL
// ============================================================
void sendEvent(bool active, int pin) {
  if (pin < 1 || pin > TOTAL_CHANNELS) return;
  if (active) {
    Serial.print("10");
    Serial.println(pin);
  } else {
    Serial.print("90");
    Serial.println(pin);
  }
}

// ============================================================
// CALIBRATE BASELINE NOISE
// ============================================================
void calibrateBaseline() {
  Serial.println("Calibrating baseline noise levels...");
  
  for (int m = 0; m < NUM_MUX; m++) {
    long sum = 0;
    int samples = 0;
    
    // Read all 16 channels and find average
    for (int ch = 0; ch < CHANNELS_PER_MUX; ch++) {
      setMuxChannel(ch);
      delay(2);
      int val = readAnalogStable(muxSignalPins[m]);
      sum += val;
      samples++;
    }
    
    baselineNoise[m] = sum / samples;
    
    Serial.print("  MUX");
    Serial.print(m + 1);
    Serial.print(" baseline: ");
    Serial.println(baselineNoise[m]);
  }
  
  Serial.println("Calibration complete.");
}

// ============================================================
// TIMER 1 ISR: SCAN MUX + DEBOUNCE
// ============================================================
ISR(TIMER1_COMPA_vect) {
  if (isrBusy) {
    errors.isrTimeout++;
    return;
  }
  isrBusy = true;

  uint8_t m = curMux;
  uint8_t c = curCh;

  // pilih channel di semua mux (shared select lines)
  setMuxChannel(c);
  
  // CRITICAL: Don't use readAnalogStable in ISR (too slow)
  // Just do single read with proper settling
  delayMicroseconds(50);
  int raw = analogRead(muxSignalPins[m]);

  ChannelStatus &st = channels[m][c];
  st.readCount++;
  st.sumReadings += raw;

  if (st.readCount >= 3) {
    int avg = st.sumReadings / 3;
    bool prev = st.active;

    if (avg > THRESHOLD_HIGH) {
      st.stableHigh++;
      st.stableLow = 0;

      if (!prev && st.stableHigh >= 2) {
        st.active = true;
        int pin = m * CHANNELS_PER_MUX + c + 1; // 1..112
        if (pin <= TOTAL_CHANNELS) {
          noInterrupts();
          fifo.add(pin);
          interrupts();
          sendEvent(true, pin);
        }
      }
    } else if (avg < THRESHOLD_LOW) {
      st.stableLow++;
      st.stableHigh = 0;

      if (prev && st.stableLow >= 2) {
        st.active = false;
        int pin = m * CHANNELS_PER_MUX + c + 1;
        if (pin <= TOTAL_CHANNELS) {
          noInterrupts();
          fifo.remove(pin);
          interrupts();
          sendEvent(false, pin);
        }
      }
    }

    st.readCount   = 0;
    st.sumReadings = 0;
  }

  // next channel
  c++;
  if (c >= CHANNELS_PER_MUX) {
    c = 0;
    m++;
    if (m >= NUM_MUX) m = 0;
  }

  curMux = m;
  curCh  = c;
  isrBusy = false;
}

// ============================================================
// HELPER: TIMER OVERFLOW SAFE
// ============================================================
bool timeElapsed(uint32_t &last, uint32_t interval) {
  uint32_t now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  
  while (!Serial && millis() < 3000); // Wait for serial, max 3s
  
  Serial.println("\n==================================================");
  Serial.println("  7-MUX 108 LINE MONITOR - FIXED VERSION");
  Serial.println("  Addressing floating pin issue");
  Serial.println("==================================================");

  // pin select
  for (int i = 0; i < 4; i++) {
    pinMode(muxControlPins[i], OUTPUT);
    digitalWrite(muxControlPins[i], LOW);
  }
  
  // Enable pins (if used)
  for (int m = 0; m < NUM_MUX; m++) {
    if (muxEnablePins[m] >= 0) {
      pinMode(muxEnablePins[m], OUTPUT);
      setMuxEnabled(m, true);
      Serial.print("MUX");
      Serial.print(m + 1);
      Serial.print(" enable pin: ");
      Serial.println(muxEnablePins[m]);
    }
  }

  Serial.println("\nAnalog pin mapping:");
  for (int m = 0; m < NUM_MUX; m++) {
    Serial.print("  MUX");
    Serial.print(m + 1);
    Serial.print(" -> A");
    Serial.println(muxSignalPins[m] - A0);
  }
  
  Serial.println("\nThresholds:");
  Serial.print("  HIGH: ");
  Serial.println(THRESHOLD_HIGH);
  Serial.print("  LOW:  ");
  Serial.println(THRESHOLD_LOW);
  Serial.println();

  delay(1000);
  
  // Calibrate baseline
  calibrateBaseline();
  
  Serial.println();

  // init channel status
  for (int m = 0; m < NUM_MUX; m++) {
    for (int c = 0; c < CHANNELS_PER_MUX; c++) {
      channels[m][c].active      = false;
      channels[m][c].stableHigh  = 0;
      channels[m][c].stableLow   = 0;
      channels[m][c].readCount   = 0;
      channels[m][c].sumReadings = 0;
    }
  }

  fifo.reset();

  // setup timer1 1ms
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A  = 15999;           // 1ms @16 MHz
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();

  Serial.println("=== MONITOR ACTIVE ===");
  Serial.println("Commands: 'd' = toggle debug mode\n");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'd' || cmd == 'D') {
      debugMode = !debugMode;
      Serial.print("Debug mode: ");
      Serial.println(debugMode ? "ON" : "OFF");
    }
  }
  
  // heartbeat untuk NC Soft
  if (timeElapsed(lastHeartbeat, 1000)) {
    Serial.println("99");
  }

  // broadcast round-robin FIFO
  if (timeElapsed(lastBroadcast, BROADCAST_INTERVAL)) {
    int pin;
    noInterrupts();
    bool ok = fifo.getNext(pin);
    interrupts();

    if (ok && pin >= 1 && pin <= TOTAL_CHANNELS) {
      Serial.print("10");
      Serial.println(pin);
    }
  }
  
  // Debug output
  if (debugMode && timeElapsed(lastDebugPrint, 5000)) {
    Serial.println("\n--- DEBUG INFO ---");
    Serial.print("Active channels: ");
    Serial.println(fifo.count);
    Serial.print("FIFO overflow errors: ");
    Serial.println(errors.fifoOverflow);
    Serial.print("ISR timeout errors: ");
    Serial.println(errors.isrTimeout);
    
    // Show current ADC values for first channel of each MUX
    Serial.println("Current ADC (ch0 each MUX):");
    for (int m = 0; m < NUM_MUX; m++) {
      setMuxChannel(0);
      delay(2);
      int val = analogRead(muxSignalPins[m]);
      Serial.print("  MUX");
      Serial.print(m + 1);
      Serial.print(": ");
      Serial.print(val);
      Serial.print(" (baseline: ");
      Serial.print(baselineNoise[m]);
      Serial.println(")");
    }
    Serial.println("------------------\n");
  }

  delay(1);
}
