// ============================================================
//  MULTIPLEXER MONITOR 7×16 – 24/7 ROBUST EDITION + LCD 1602A
//  Fitur: Monitoring pin aktif dan data serial pada LCD I2C
//  Dual Serial Output: Serial0 (USB) dan Serial2 (TX2/RX2)
// ============================================================
#include <avr/wdt.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define NUM_MUX 7
#define CHANNELS_PER_MUX 16
#define TOTAL_CHANNELS 108
#define FIFO_SIZE 16
#define LED_PIN 13
#define EEPROM_HOUR_ADDR 0
#define CRC_POLY 0x8C

// Inisialisasi LCD I2C (alamat 0x27, 16 kolom, 2 baris)
// Jika tidak bekerja, coba alamat 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int THRESHOLD_HIGH = 150;
const int THRESHOLD_LOW = 100;
const unsigned long BROADCAST_INTERVAL = 2500;
const unsigned long HEARTBEAT_INTERVAL = 5000;
const unsigned long BASELINE_RECAL_HOURS = 24;
const unsigned long ISR_HANG_TIMEOUT = 500;
const unsigned long LCD_UPDATE_INTERVAL = 500;  // Update LCD setiap 500ms

// ---------- global timing ----------
uint32_t lastHeartbeat = 0, lastBroadcast = 0, lastStatusReport = 0,
         lastDebugPrint = 0, lastIsrCheck = 0, lastBaselineRecal = 0,
         lastLcdUpdate = 0;
bool baselineRecalRequested = false;

// ---------- LCD display variables ----------
String lastSerialData = "";
int lastActivePin = 0;
int activeCount = 0;

// ---------- pin mapping ----------
const byte muxControlPins[4] = { 42, 43, 44, 45 };
const byte muxSignalPins[NUM_MUX] = { A0, A1, A2, A3, A4, A5, A6 };
const byte muxEnablePins[NUM_MUX] = { -1, -1, -1, -1, -1, -1, -1 };

// ---------- channel status ----------
struct ChannelStatus {
  bool active;
  uint8_t stableHigh, stableLow, readCount;
  uint16_t sumReadings;
  int lastAnalogValue;
};
ChannelStatus channels[NUM_MUX][CHANNELS_PER_MUX];

// ---------- FIFO ----------
struct SafeFIFO {
  int buffer[FIFO_SIZE];
  int count, index;
  void reset() {
    count = index = 0;
  }
  bool add(int pin);
  bool remove(int pin);
  bool getNext(int &pin);
};
SafeFIFO fifo;

// ---------- error counters ----------
struct {
  uint32_t fifoOverflow, isrTimeout, adcStuck, crcError, recalDone;
} errors;

// ---------- scan pointer ----------
volatile uint8_t curMux = 0, curCh = 0;
volatile bool isrBusy = false;
volatile uint32_t isrLastMillis = 0;

// ---------- baseline ----------
int baselineNoise[NUM_MUX];

// ---------- util ----------
bool timeElapsed(uint32_t &last, uint32_t interval) {
  uint32_t now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
}

// ---------- CRC ----------
uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = crc & 0x80 ? (crc << 1) ^ CRC_POLY : crc << 1;
  }
  return crc;
}

// ---------- LED ----------
void ledBlink(int times, int on = 150, int off = 150) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(on);
    digitalWrite(LED_PIN, LOW);
    delay(off);
  }
}

// ---------- DUAL SERIAL PRINT ----------
void dualPrint(const String &msg) {
  Serial.print(msg);
  Serial2.print(msg);
}

void dualPrintln(const String &msg) {
  Serial.println(msg);
  Serial2.println(msg);
}

void dualPrintln(const __FlashStringHelper *msg) {
  Serial.println(msg);
  Serial2.println(msg);
}

// ---------- LCD UPDATE ----------
void updateLCD() {
  lcd.clear();
  
  // Baris 1: Pin aktif dan jumlah
  lcd.setCursor(0, 0);
  if (lastActivePin > 0) {
    lcd.print("Pin:");
    lcd.print(lastActivePin);
    lcd.print(" Act:");
    lcd.print(activeCount);
  } else {
    lcd.print("No Active Pin");
  }
  
  // Baris 2: Data serial terakhir
  lcd.setCursor(0, 1);
  if (lastSerialData.length() > 0) {
    // Tampilkan maksimal 16 karakter
    lcd.print(lastSerialData.substring(0, 16));
  } else {
    lcd.print("Waiting data...");
  }
}

// ---------- IMPROVED ANALOG ----------
int readAnalogStable(int pin) {
  analogRead(pin);
  delayMicroseconds(50);
  int s = 0;
  for (int i = 0; i < 3; i++) {
    s += analogRead(pin);
    delayMicroseconds(20);
  }
  int v = s / 3;
  if (v == 0 || v == 1023) errors.adcStuck++;
  return v;
}

// ---------- MUX CONTROL ----------
void setMuxChannel(uint8_t ch) {
  for (int i = 0; i < 4; i++) digitalWrite(muxControlPins[i], bitRead(ch, i));
  delayMicroseconds(20);
}
void setMuxEnabled(uint8_t m, bool en) {
  if (muxEnablePins[m] >= 0) digitalWrite(muxEnablePins[m], en ? LOW : HIGH);
}

// ---------- SERIAL WITH CRC ----------
void sendSerial(const __FlashStringHelper *prefix, int val) {
  char buf[16];
  sprintf_P(buf, (const char *)prefix, val);
  uint8_t crc = crc8((uint8_t *)buf, strlen(buf));
  
  // Kirim ke kedua serial port
  Serial.println(buf);
  Serial2.println(buf);
  
  // Update LCD data
  lastSerialData = String(buf);
}

void sendEvent(bool active, int pin, int analogValue) {
  if (pin < 1 || pin > TOTAL_CHANNELS) return;
  
  String data = "";
  
  if (active) {
    data = "10" + String(pin) + ":" + String(analogValue);
    // Kirim ke kedua serial port
    Serial.println(data);
    Serial2.println(data);
    lastActivePin = pin;
    activeCount = fifo.count;
  } else {
    data = "90" + String(pin);
    // Kirim ke kedua serial port
    Serial.println(data);
    Serial2.println(data);
    if (lastActivePin == pin) lastActivePin = 0;
    activeCount = fifo.count;
  }
  
  // Update LCD data
  lastSerialData = data;
}

// ---------- BASELINE CALIBRATE ----------
void calibrateBaseline(bool silent = false) {
  if (!silent) {
    dualPrintln(F("Calibrating baseline..."));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
  }
  
  for (int m = 0; m < NUM_MUX; m++) {
    long sum = 0;
    for (int c = 0; c < CHANNELS_PER_MUX; c++) {
      setMuxChannel(c);
      delay(2);
      sum += readAnalogStable(muxSignalPins[m]);
    }
    baselineNoise[m] = sum / CHANNELS_PER_MUX;
  }
  errors.recalDone++;
  
  if (!silent) {
    dualPrintln(F("Calibration done."));
    lcd.setCursor(0, 1);
    lcd.print("Done!");
    delay(1000);
  }
}

// ---------- FIFO IMPLEMENTATION ----------
bool SafeFIFO::add(int pin) {
  if (count >= FIFO_SIZE) {
    errors.fifoOverflow++;
    return false;
  }
  for (int i = 0; i < count; i++)
    if (buffer[i] == pin) return true;
  buffer[count++] = pin;
  return true;
}
bool SafeFIFO::remove(int pin) {
  if (count == 0) return false;
  int pos = -1;
  for (int i = 0; i < count; i++)
    if (buffer[i] == pin) {
      pos = i;
      break;
    }
  if (pos < 0) return false;
  for (int i = pos; i < count - 1; i++) buffer[i] = buffer[i + 1];
  count--;
  if (index >= count) index = 0;
  return true;
}
bool SafeFIFO::getNext(int &pin) {
  if (count == 0) return false;
  if (index >= count) index = 0;
  pin = buffer[index++];
  return true;
}

// ---------- TIMER1 ISR ----------
ISR(TIMER1_COMPA_vect) {
  isrLastMillis = millis();
  if (isrBusy) {
    errors.isrTimeout++;
    return;
  }
  isrBusy = true;

  uint8_t m = curMux, c = curCh;
  if (m >= NUM_MUX || c >= CHANNELS_PER_MUX) {
    isrBusy = false;
    return;
  }

  setMuxChannel(c);
  delayMicroseconds(50);
  int raw = analogRead(muxSignalPins[m]);

  ChannelStatus &st = channels[m][c];
  st.readCount++;
  st.sumReadings += raw;

  if (st.readCount >= 3) {
    int avg = st.sumReadings / 3;
    bool prev = st.active;
    st.lastAnalogValue = avg;

    if (avg > THRESHOLD_HIGH) {
      st.stableHigh++;
      st.stableLow = 0;
      if (!prev && st.stableHigh >= 2) {
        st.active = true;
        int pin = m * CHANNELS_PER_MUX + c + 1;
        if (pin <= TOTAL_CHANNELS) {
          noInterrupts();
          fifo.add(pin);
          interrupts();
          sendEvent(true, pin, avg);
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
          sendEvent(false, pin, 0);
        }
      }
    }
    st.readCount = 0;
    st.sumReadings = 0;
  }

  c++;
  if (c >= CHANNELS_PER_MUX) {
    c = 0;
    m++;
    if (m >= NUM_MUX) m = 0;
  }
  curMux = m;
  curCh = c;
  isrBusy = false;
}

// ---------- SETUP ----------
void setup() {
  wdt_disable();
  pinMode(LED_PIN, OUTPUT);
  ledBlink(1);
  
  // Inisialisasi Serial0 (USB)
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  // Inisialisasi Serial2 (TX2/RX2 - Pin 16/17 pada Mega)
  Serial2.begin(115200);
  
  // Inisialisasi LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("7-MUX Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  dualPrintln(F("\n=== 7-MUX 108-CH 24/7 ROBUST ==="));
  dualPrintln(F("=== Created by: WahyuCF, RAffi, Orang Pintar ==="));
  dualPrintln(F("=== September 2025 ==="));
  dualPrintln(F("=== Dual Serial Output Active ===\n"));

  for (int i = 0; i < 4; i++) pinMode(muxControlPins[i], OUTPUT);
  for (int m = 0; m < NUM_MUX; m++) {
    if (muxEnablePins[m] >= 0) pinMode(muxEnablePins[m], OUTPUT);
    setMuxEnabled(m, true);
  }

  calibrateBaseline();
  fifo.reset();
  for (int m = 0; m < NUM_MUX; m++)
    for (int c = 0; c < CHANNELS_PER_MUX; c++)
      channels[m][c] = { false, 0, 0, 0, 0, 0 };

  // Timer1 1ms
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 15999;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();

  // EEPROM cek last cal hour
  uint32_t lastCalHour = 0;
  EEPROM.get(EEPROM_HOUR_ADDR, lastCalHour);
  uint32_t nowHour = millis() / 3600000UL;
  if (nowHour - lastCalHour >= BASELINE_RECAL_HOURS) {
    baselineRecalRequested = true;
    EEPROM.put(EEPROM_HOUR_ADDR, nowHour);
  }

  dualPrintln(F("=== MONITOR ACTIVE ==="));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monitor Active");
  delay(1000);
  
  wdt_enable(WDTO_1S);
}

// ---------- LOOP ----------
void loop() {
  wdt_reset();

  // ISR hang detector
  if (timeElapsed(lastIsrCheck, 100)) {
    if (millis() - isrLastMillis > ISR_HANG_TIMEOUT) {
      dualPrintln(F("ISR hang detected"));
      ledBlink(3);
    }
  }

  // Update LCD
  if (timeElapsed(lastLcdUpdate, LCD_UPDATE_INTERVAL)) {
    updateLCD();
  }

  // Serial command (cek dari kedua serial port)
  char c = 0;
  if (Serial.available()) {
    c = Serial.read();
  } else if (Serial2.available()) {
    c = Serial2.read();
  }
  
  if (c == 'd' || c == 'D') {
    static bool debugMode = false;
    debugMode = !debugMode;
    dualPrint(F("Debug "));
    dualPrintln(debugMode ? F("ON") : F("OFF"));
    if (debugMode) {
      dualPrint(F("FIFO cnt "));
      dualPrintln(String(fifo.count));
      dualPrint(F("Err ovf/timeout/stuck/crc/recal "));
      String errMsg = String(errors.fifoOverflow) + "/" + 
                      String(errors.isrTimeout) + "/" + 
                      String(errors.adcStuck) + "/" + 
                      String(errors.crcError) + "/" + 
                      String(errors.recalDone);
      dualPrintln(errMsg);
    }
  }

  // Heartbeat
  if (timeElapsed(lastHeartbeat, HEARTBEAT_INTERVAL)) {
    sendSerial(F("99"), 0);
    ledBlink(1, 50, 50);
  }

  // Broadcast FIFO
  if (timeElapsed(lastBroadcast, BROADCAST_INTERVAL)) {
    int pin;
    noInterrupts();
    bool ok = fifo.getNext(pin);
    interrupts();
    if (ok && pin >= 1 && pin <= TOTAL_CHANNELS) {
      int m = (pin - 1) / CHANNELS_PER_MUX;
      int c = (pin - 1) % CHANNELS_PER_MUX;
      if (m < NUM_MUX && c < CHANNELS_PER_MUX) {
        int analogVal = channels[m][c].lastAnalogValue;
        sendEvent(true, pin, analogVal);
      }
    }
  }

  // 24h baseline recal
  if (baselineRecalRequested) {
    baselineRecalRequested = false;
    calibrateBaseline(true);
  }

  delay(1);
}
