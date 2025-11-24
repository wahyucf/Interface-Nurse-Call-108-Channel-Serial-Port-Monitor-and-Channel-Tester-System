// ============================================================
//  MULTIPLEXER MONITOR 7×16 – 24/7 ROBUST EDITION
//  Fitur identik dengan versi sebelumnya, hanya tambahan:
//   - Watchdog, CRC, baseline auto-recal, LED status, dll.
//   - NILAI ANALOG pada setiap pin aktif
// ============================================================
#include <avr/wdt.h>
#include <EEPROM.h>

#define NUM_MUX 7
#define CHANNELS_PER_MUX 16
#define TOTAL_CHANNELS 108
#define FIFO_SIZE 16
#define LED_PIN 13          // status LED
#define EEPROM_HOUR_ADDR 0  // byte ke-0..3 untuk last-cal time
#define CRC_POLY 0x8C       // CRC-8 Maxim

const int THRESHOLD_HIGH = 150;
const int THRESHOLD_LOW = 100;
const unsigned long BROADCAST_INTERVAL = 2500;
const unsigned long HEARTBEAT_INTERVAL = 5000;
const unsigned long BASELINE_RECAL_HOURS = 24;
const unsigned long ISR_HANG_TIMEOUT = 500;  // ms

// ---------- global timing ----------
uint32_t lastHeartbeat = 0, lastBroadcast = 0, lastStatusReport = 0,
         lastDebugPrint = 0, lastIsrCheck = 0, lastBaselineRecal = 0;
bool baselineRecalRequested = false;

// ---------- pin mapping ----------
const byte muxControlPins[4] = { 42, 43, 44, 45 };
const byte muxSignalPins[NUM_MUX] = { A0, A1, A2, A3, A4, A5, A6 };
const byte muxEnablePins[NUM_MUX] = { -1, -1, -1, -1, -1, -1, -1 };

// ---------- channel status ----------
struct ChannelStatus {
  bool active;
  uint8_t stableHigh, stableLow, readCount;
  uint16_t sumReadings;
  int lastAnalogValue;  // TAMBAHAN: simpan nilai analog terakhir
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

// ---------- IMPROVED ANALOG ----------
int readAnalogStable(int pin) {
  analogRead(pin);  // discard
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
  Serial.println(buf);
  //Serial.print(':');              // delimiter
  //Serial.println(crc, HEX);
}

// MODIFIKASI: Tambahkan parameter analogValue
void sendEvent(bool active, int pin, int analogValue) {
  if (pin < 1 || pin > TOTAL_CHANNELS) return;
  
  // Format: 10<pin>:<analogValue> atau 90<pin>
  if (active) {
    Serial.print(F("10"));
    Serial.print(pin);
    Serial.print(':');
    Serial.println(analogValue);
  } else {
    Serial.print(F("90"));
    Serial.println(pin);
  }
}

// ---------- BASELINE CALIBRATE ----------
void calibrateBaseline(bool silent = false) {
  if (!silent) Serial.println(F("Calibrating baseline..."));
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
  if (!silent) Serial.println(F("Calibration done."));
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

    // TAMBAHAN: Simpan nilai analog terakhir
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
          sendEvent(true, pin, avg);  // KIRIM DENGAN NILAI ANALOG
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
          sendEvent(false, pin, 0);  // DEAKTIVASI tanpa nilai analog
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
  wdt_disable();  // biar saat upload aman
  pinMode(LED_PIN, OUTPUT);
  ledBlink(1);
  Serial.begin(115200);
  while (!Serial && millis() < 3000)
    ;
  Serial.println(F("\n=== 7-MUX 108-CH 24/7 ROBUST ==="));
  Serial.println(F("=== Created by: WahyuCF, RAffi, Orang Pintar ==="));
  Serial.println(F("=== September 2025 ===\n"));

  for (int i = 0; i < 4; i++) pinMode(muxControlPins[i], OUTPUT);
  for (int m = 0; m < NUM_MUX; m++) {
    if (muxEnablePins[m] >= 0) pinMode(muxEnablePins[m], OUTPUT);
    setMuxEnabled(m, true);
  }

  calibrateBaseline();
  fifo.reset();
  for (int m = 0; m < NUM_MUX; m++)
    for (int c = 0; c < CHANNELS_PER_MUX; c++)
      channels[m][c] = { false, 0, 0, 0, 0, 0 };  // tambah field lastAnalogValue

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

  Serial.println(F("=== MONITOR ACTIVE ==="));
  wdt_enable(WDTO_1S);  // 1s watchdog
}

// ---------- LOOP ----------
void loop() {
  wdt_reset();  // feed dog

  // ISR hang detector
  if (timeElapsed(lastIsrCheck, 100)) {
    if (millis() - isrLastMillis > ISR_HANG_TIMEOUT) {
      Serial.println(F("ISR hang detected"));
      ledBlink(3);
    }
  }

  // Serial command
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'd' || c == 'D') {
      static bool debugMode = false;
      debugMode = !debugMode;
      Serial.print(F("Debug "));
      Serial.println(debugMode ? F("ON") : F("OFF"));
      if (debugMode) {
        Serial.print(F("FIFO cnt "));
        Serial.println(fifo.count);
        Serial.print(F("Err ovf/timeout/stuck/crc/recal "));
        Serial.print(errors.fifoOverflow);
        Serial.print('/');
        Serial.print(errors.isrTimeout);
        Serial.print('/');
        Serial.print(errors.adcStuck);
        Serial.print('/');
        Serial.print(errors.crcError);
        Serial.print('/');
        Serial.println(errors.recalDone);
      }
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
      // TAMBAHAN: Ambil nilai analog dari channel yang aktif
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
