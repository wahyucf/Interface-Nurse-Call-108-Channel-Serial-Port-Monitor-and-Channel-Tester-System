// =====================================================
//  LCD 12864 ST7920 PARALLEL 8-BIT — SERIAL TERMINAL
//  Auto-scroll + Line Number + Clear & Reset Commands
//  Arduino Mega 2560
// =====================================================

// Pin LCD
#define LCD_RS 22
#define LCD_CS 23
#define LCD_E 24
#define LCD_D0 25
#define LCD_D1 26
#define LCD_D2 27
#define LCD_D3 28
#define LCD_D4 29
#define LCD_D5 30
#define LCD_D6 31
#define LCD_D7 32

// Pin tombol
#define PIN_CLEAR 40     // tekan LOW → LCD clear
#define PIN_RESET 41     // tekan LOW → reset nomor baris

// ===================== PROTOTYPE =====================
void lcdWriteCommand(uint8_t cmd);
void lcdWriteData(uint8_t data);
void lcdSetXY(uint8_t x, uint8_t y);
void lcdPrint(const char *s);
void lcdClear();

// ===================== BUFFER =====================
String lineBuffer[4];
const int LCD_COLS = 16;

unsigned long lineNumber = 1;   // nomor baris meningkat terus

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_E, OUTPUT);
  for (int pin = LCD_D0; pin <= LCD_D7; pin++) {
    pinMode(pin, OUTPUT);
  }

  pinMode(PIN_CLEAR, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);

  digitalWrite(LCD_CS, LOW);
  delay(50);

  // Init LCD
  lcdWriteCommand(0x30);
  delay(2);
  lcdWriteCommand(0x30);
  delay(2);
  lcdWriteCommand(0x0C);
  lcdWriteCommand(0x01);
  delay(5);

  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("LCD Ready...");
  lcdSetXY(0, 1);
  lcdPrint("BR:9600");
  
  lcdSetXY(0, 2);
  lcdPrint("By Wahyu CF");
  
  lcdSetXY(0, 3);
  lcdPrint("23 Dec 2025");
  delay(2000);
  lcdClear();
  
  lcdSetXY(0, 0);
  lcdPrint("Tester Ready...");
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  // ================= PIN COMMAND =================
  if (digitalRead(PIN_CLEAR) == LOW) {
    lcdClear();
    for (int i = 0; i < 4; i++) lineBuffer[i] = "";
    delay(300);
  }

  if (digitalRead(PIN_RESET) == LOW) {
    lineNumber = 1;
    delay(300);
  }

  // ================= SERIAL COMMAND =================
  if (Serial.available()) {

    String incoming = Serial.readStringUntil('\n');
    incoming.trim();

    if (incoming.equalsIgnoreCase("CLEAR")) {
      lcdClear();
      for (int i = 0; i < 4; i++) lineBuffer[i] = "";
      return;
    }

    if (incoming.equalsIgnoreCase("RESET")) {
      lineNumber = 1;
      return;
    }

    if (incoming.length() == 0) return;

    // ================= TAMBAHKAN NOMOR BARIS =================
    char numBuf[10];
    sprintf(numBuf, "%03lu", lineNumber);  // format: 001, 002, 003 ...

    String finalLine = String(numBuf) + ":" + incoming;
    lineNumber++;

    // ================= AUTO SCROLL =================
    for (int k = 0; k < 3; k++) {
      lineBuffer[k] = lineBuffer[k + 1];
    }
    lineBuffer[3] = finalLine;

    // ================= CETAK KE LCD =================
    for (int row = 0; row < 4; row++) {

      lcdSetXY(0, row);

      // clear row
      for (int c = 0; c < LCD_COLS; c++) {
        lcdWriteData(' ');
      }

      lcdSetXY(0, row);
      lcdPrint(lineBuffer[row].c_str());
    }
  }
}

// =====================================================
// LCD LOW-LEVEL FUNCTIONS
// =====================================================
void lcdWriteCommand(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(LCD_D0 + i, (cmd >> i) & 1);
  }
  digitalWrite(LCD_E, HIGH);
  delayMicroseconds(2);
  digitalWrite(LCD_E, LOW);
  delayMicroseconds(2);
}

void lcdWriteData(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  for (int i = 0; i < 8; i++) {
    digitalWrite(LCD_D0 + i, (data >> i) & 1);
  }
  digitalWrite(LCD_E, HIGH);
  delayMicroseconds(2);
  digitalWrite(LCD_E, LOW);
  delayMicroseconds(2);
}

void lcdSetXY(uint8_t x, uint8_t y) {
  uint8_t addr = x + 0x80;
  if (y == 1) addr = x + 0x90;
  if (y == 2) addr = x + 0x88;
  if (y == 3) addr = x + 0x98;
  lcdWriteCommand(addr);
}

void lcdPrint(const char *s) {
  while (*s) {
    lcdWriteData(*s++);
  }
}

void lcdClear() {
  lcdWriteCommand(0x01);
  delay(2);
}
