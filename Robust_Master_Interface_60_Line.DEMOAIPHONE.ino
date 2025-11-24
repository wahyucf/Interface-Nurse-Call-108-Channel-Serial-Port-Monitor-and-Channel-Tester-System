// Created by WahyuCF 2022
// DEMO AIPHONE - 48 LINE System (24/7 Robust Version)
// Serial2 TX=PIN16 RX=PIN17

#include <Wire.h>
#include <avr/wdt.h>  // Watchdog timer

//============================================
// KONFIGURASI UTAMA
//============================================
const float LimitVal = 70;        // Tegangan minimum (iPhone: 150, Commax: 50)
const int JmlLine = 48;           // Jumlah line
const int TikTime = 10;           // Interval ticker
const int DispTime = 12;          // Display time
const int MUX_SETTLE_TIME = 10;   // Waktu stabilisasi mux (ms)
const int ADC_SAMPLES = 3;        // Jumlah sample ADC untuk averaging
const unsigned long HEARTBEAT_INTERVAL = 60000;  // 1 menit
const unsigned long SYSTEM_CHECK_INTERVAL = 300000; // 5 menit
//============================================

// Pin Multiplexer
const int muxPins[16] = {
  22, 23, 24, 25,  // S0-S3 (Mux1)
  26, 27, 28, 29,  // S4-S7 (Mux2)
  30, 31, 32, 33,  // S8-S11 (Mux3)
  38, 39, 40, 41   // S12-S15 (Mux4)
};

const int analogPins[4] = {A15, A14, A13, A12};  // Mux1-4 inputs
const int ledPin = 13;

// Arrays untuk sensor values
float mySensVals[71];
int mySensValsStat[71];

// Timing variables (menggunakan unsigned long untuk menghindari overflow)
unsigned long millisB = 0;
unsigned long millisC = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSystemCheck = 0;
unsigned long loopCounter = 0;
int lineOn = 0;

// System health monitoring
struct SystemHealth {
  unsigned long totalLoops;
  unsigned long errors;
  unsigned long lastReset;
  bool serialOK;
  bool serial2OK;
} health;

void setup() {
  // Disable watchdog jika aktif dari reset sebelumnya
  wdt_disable();
  
  Serial.begin(9600);
  Serial2.begin(9600);
  
  // Tunggu serial ready dengan timeout
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 3000)) {
    ; // Wait max 3 detik
  }
  
  Serial.println(F("System Starting..."));
  
  // Initialize arrays
  for (int i = 0; i < 71; i++) {
    mySensVals[i] = 0;
    mySensValsStat[i] = 1;
  }
  
  // Setup multiplexer pins
  for (int i = 0; i < 16; i++) {
    pinMode(muxPins[i], OUTPUT);
    digitalWrite(muxPins[i], LOW);
  }
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Initialize health monitoring
  health.totalLoops = 0;
  health.errors = 0;
  health.lastReset = millis();
  health.serialOK = true;
  health.serial2OK = true;
  
  // Startup sequence
  blinkLed(3, 100);
  delay(500);
  
  // Enable watchdog timer (8 second timeout)
  wdt_enable(WDTO_8S);
  
  Serial.println(F("System Ready"));
  Serial.println(F("Watchdog enabled - 8s timeout"));
  Serial2.print(F("<N>"));
  
  lastHeartbeat = millis();
  lastSystemCheck = millis();
}

void blinkLed(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(duration);
    digitalWrite(ledPin, LOW);
    if (i < times - 1) delay(duration);
  }
}

void setMuxChannel(int muxIndex, int channel) {
  if (muxIndex < 0 || muxIndex > 3) return;
  if (channel < 0 || channel > 15) return;
  
  int basePin = muxIndex * 4;
  for (int i = 0; i < 4; i++) {
    digitalWrite(muxPins[basePin + i], (channel >> i) & 0x01);
  }
}

int readMuxValue(int count) {
  if (count < 0 || count >= JmlLine) return 0;
  
  int muxIndex = count / 16;
  int channel = count % 16;
  
  setMuxChannel(muxIndex, channel);
  delay(MUX_SETTLE_TIME);
  
  // Averaging untuk mengurangi noise
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(analogPins[muxIndex]);
    if (ADC_SAMPLES > 1) delay(2);
  }
  
  return sum / ADC_SAMPLES;
}

void checkSerialHealth() {
  // Check Serial
  if (!Serial) {
    health.serialOK = false;
    health.errors++;
  } else {
    health.serialOK = true;
  }
  
  // Check Serial2
  if (!Serial2) {
    health.serial2OK = false;
    health.errors++;
    // Coba reinit Serial2
    Serial2.begin(9600);
    delay(100);
  } else {
    health.serial2OK = true;
  }
}

void printSystemHealth() {
  if (!Serial) return;
  
  Serial.println(F("\n=== System Health ==="));
  Serial.print(F("Uptime: "));
  Serial.print((millis() - health.lastReset) / 1000);
  Serial.println(F(" seconds"));
  Serial.print(F("Total Loops: "));
  Serial.println(health.totalLoops);
  Serial.print(F("Errors: "));
  Serial.println(health.errors);
  Serial.print(F("Serial: "));
  Serial.println(health.serialOK ? F("OK") : F("ERROR"));
  Serial.print(F("Serial2: "));
  Serial.println(health.serial2OK ? F("OK") : F("ERROR"));
  Serial.print(F("Free RAM: "));
  Serial.print(getFreeRAM());
  Serial.println(F(" bytes"));
  Serial.println(F("====================\n"));
}

int getFreeRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void safeSerialPrint(const char* msg) {
  if (Serial) {
    Serial.print(msg);
  }
}

void safeSerial2Print(const String& msg) {
  if (Serial2) {
    Serial2.print(msg);
  } else {
    health.errors++;
  }
}

void loop() {
  // Reset watchdog timer
  wdt_reset();
  
  lineOn = 0;
  health.totalLoops++;
  
  // System health check periodic
  unsigned long currentMillis = millis();
  if (currentMillis - lastSystemCheck >= SYSTEM_CHECK_INTERVAL) {
    checkSerialHealth();
    printSystemHealth();
    lastSystemCheck = currentMillis;
  }
  
  // Heartbeat
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    if (Serial) {
      Serial.print(F("Heartbeat - Loops: "));
      Serial.println(health.totalLoops);
    }
    blinkLed(2, 50);
    lastHeartbeat = currentMillis;
  }

  // Main scanning loop
  for (int count = 0; count < JmlLine; count++) {
    wdt_reset();  // Reset watchdog di dalam loop panjang
    
    int printdisp = count + 1;
    int value = readMuxValue(count);
    
    // Validasi pembacaan
    if (value < 0 || value > 1023) {
      health.errors++;
      continue;
    }
    
    mySensVals[count] = value;

    // Check if below limit
    if (mySensVals[count] < LimitVal) {
      if (mySensValsStat[count] == 0) {
        if (Serial) {
          Serial.print(F("90"));
          Serial.print(printdisp);
          Serial.println(F(":"));
        }
        safeSerial2Print(F("<N>"));
        mySensValsStat[count] = 1;
        lineOn = 0;
        millisC = 0;
        delay(10);
      }
      continue;
    }

    // Process active line
    if ((millisC >= DispTime) || (millisC == 0)) {
      if (Serial) {
        Serial.print(F("10"));
        Serial.print(printdisp);
        Serial.print(F(": "));
        Serial.println(mySensVals[count]);
      }
      
      // Kirim nomor kamar dengan boundary check
      if (printdisp >= 0 && printdisp < 71) {
        safeSerial2Print(KirimDisply[printdisp]);
      }
      
      delay(10);
      millisC = 0;
      blinkLed(2, 100);
    }
    
    millisC++;
    mySensValsStat[count] = 0;
    lineOn = 1;
  }

  blinkLed(1, 200);

  // Ticker untuk idle state
  millisB++;
  if ((millisB >= TikTime) && (lineOn <= 0)) {
    if (Serial) Serial.println(F("99:"));
    safeSerial2Print(F("<N>"));
    delay(5);
    millisB = 0;
  }
  
  // Prevent overflow pada counter (reset setiap ~49 hari)
  if (millisB > 4000000000UL) millisB = 0;
  if (millisC > 4000000000UL) millisC = 0;
}

// Nomor Kamar (PIN-1 sampai PIN-70)
const String KirimDisply[] PROGMEM = {
  "<N>",          // PIN-0
  "<RR101-01>",   "<KKM-101>",    "<RR102-01>",   "<KKM-102>",
  "<RR103-01>",   "<KKM-103>",    "<RR104-01>",   "<KKM-104>",
  "<RR105-01>",   "<KKM-105>",    "<RR106-01>",   "<KKM-106>",
  "<RR107-01>",   "<KKM-107>",    "<RR108-01>",   "<KKM-108>",
  "<RR109-01>",   "<KKM-109>",    "<RR110-01>",   "<KKM-110>",
  "<RR111-01>",   "<KKM-111>",    "<RR112-01>",   "<KKM-112>",
  "<RR113-01>",   "<KKM-113>",    "<RR114-01>",   "<KKM-114>",
  "<RR115-01>",   "<KKM-115>",    "<RR116-01>",   "<KKM-116>",
  "<RR117-01>",   "<KKM-117>",    "<RR118-01>",   "<KKM-118>",
  "<RR119-01>",   "<KKM-119>",    "<RR120-01>",   "<KKM-120>",
  "<RR121-01>",   "<KKM-121>",    "<RR122-01>",   "<KKM-122>",
  "<RR123-01>",   "<KKM-123>",    "<RR124-01>",   "<KKM-124>",
  "<RR125-01>",   "<KKM-125>",    "<RR126-01>",   "<KKM-126>",
  "<RR127-01>",   "<KKM-127>",    "<RR.M1-01>",   "<RR.M1-02>",
  "<RR.M1-03>",   "<RR.M1-04>",   "<KKM-M1>",     "<CCB-NOL>",
  "<RR61>",       "<KKM62>",      "<RR63>",       "<KKM64>",
  "<RR65>",       "<RR66>",       "<RR67>",       "<RR68>",
  "<KKM69>",      "<CCB70>"
};
