// =====================================================
//  LCD 12864 ST7920 PARALLEL 8-BIT — SERIAL TERMINAL
//  Auto-scroll + Line Number + Clear & Reset Commands
//  Arduino Mega 2560
//  + Baud Rate Selector via Serial (Saved to EEPROM)
//  + Startup Menu untuk Setting Baud Rate
// =====================================================

#include <EEPROM.h>

// EEPROM Address
#define EEPROM_ADDR_BAUD 0    // 4 bytes untuk menyimpan baud rate (unsigned long)
#define EEPROM_ADDR_VALID 4   // 1 byte untuk validasi (0xAA = valid)

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
void saveBaudRateToEEPROM(unsigned long baud);
unsigned long loadBaudRateFromEEPROM();
bool isValidBaudRate(unsigned long baud);
void showBaudRateMenu();
void changeBaudRate(unsigned long newBaud);

// ===================== BUFFER =====================
String lineBuffer[4];
const int LCD_COLS = 16;

unsigned long lineNumber = 1;   // nomor baris meningkat terus
unsigned long currentBaudRate = 115200;  // baud rate default (akan di-load dari EEPROM)

// Daftar baud rate yang tersedia
const unsigned long baudRates[] = {300, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200};
const int numBaudRates = 11;

// =====================================================
// SETUP
// =====================================================
void setup() {
  // Load baud rate dari EEPROM
  unsigned long savedBaud = loadBaudRateFromEEPROM();
  if (savedBaud != 0) {
    currentBaudRate = savedBaud;
  }
  
  Serial.begin(currentBaudRate);

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
  char brDisplay[17];
  sprintf(brDisplay, "BR:%lu", currentBaudRate);
  lcdPrint(brDisplay);
  
  lcdSetXY(0, 2);
  lcdPrint("By Wahyu CF");
  
  lcdSetXY(0, 3);
  lcdPrint("23 Dec 2025");
  delay(2000);
  
  // ============== STARTUP MENU ==============
  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("Change BR?");
  lcdSetXY(0, 1);
  lcdPrint("Send 'MENU'");
  lcdSetXY(0, 2);
  lcdPrint("in 5 seconds...");
  
  Serial.println();
  Serial.println("=====================================");
  Serial.println("   LCD SERIAL TERMINAL READY");
  Serial.println("=====================================");
  Serial.print("Current Baud Rate: ");
  Serial.println(currentBaudRate);
  Serial.println();
  Serial.println("Want to change Baud Rate?");
  Serial.println("Send 'MENU' within 5 seconds...");
  Serial.println("=====================================");
  
  // Tunggu 5 detik untuk menu
  unsigned long startTime = millis();
  bool menuRequested = false;
  
  while (millis() - startTime < 5000) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd.equalsIgnoreCase("MENU")) {
        menuRequested = true;
        break;
      }
    }
    
    // Update countdown di LCD
    int remaining = 5 - ((millis() - startTime) / 1000);
    lcdSetXY(14, 2);
    char countBuf[3];
    sprintf(countBuf, "%d", remaining);
    lcdPrint(countBuf);
  }
  
  if (menuRequested) {
    showBaudRateMenu();
  } else {
    lcdClear();
    lcdSetXY(0, 0);
    lcdPrint("Tester Ready...");
    Serial.println();
    Serial.println("Terminal Mode Active!");
    Serial.println("Commands: CLEAR, RESET, MENU, BAUD:xxxxx");
    Serial.println("=====================================");
    Serial.println();
  }
}

// =====================================================
// MENU BAUD RATE
// =====================================================
void showBaudRateMenu() {
  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("SELECT BAUD RATE");
  
  Serial.println();
  Serial.println("=====================================");
  Serial.println("      BAUD RATE SELECTION MENU");
  Serial.println("=====================================");
  
  for (int i = 0; i < numBaudRates; i++) {
    Serial.print("[");
    Serial.print(i + 1);
    Serial.print("] ");
    Serial.print(baudRates[i]);
    Serial.print(" bps");
    if (baudRates[i] == currentBaudRate) {
      Serial.print(" (Current)");
    }
    Serial.println();
  }
  
  Serial.println("[0] Cancel / Keep Current");
  Serial.println("=====================================");
  Serial.print("Select option (0-");
  Serial.print(numBaudRates);
  Serial.print("): ");
  
  lcdSetXY(0, 1);
  lcdPrint("Check Serial");
  lcdSetXY(0, 2);
  lcdPrint("Monitor for");
  lcdSetXY(0, 3);
  lcdPrint("options...");
  
  // Tunggu input dari user
  bool validInput = false;
  while (!validInput) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      int choice = input.toInt();
      
      if (choice == 0) {
        Serial.println("0");
        Serial.println();
        Serial.println("Cancelled. Keeping current baud rate.");
        Serial.print("Current: ");
        Serial.println(currentBaudRate);
        
        lcdClear();
        lcdSetXY(0, 0);
        lcdPrint("Cancelled");
        lcdSetXY(0, 1);
        char brBuf[17];
        sprintf(brBuf, "Keep: %lu", currentBaudRate);
        lcdPrint(brBuf);
        delay(2000);
        
        lcdClear();
        lcdSetXY(0, 0);
        lcdPrint("Tester Ready...");
        
        Serial.println("=====================================");
        Serial.println("Terminal Mode Active!");
        Serial.println("=====================================");
        Serial.println();
        
        validInput = true;
      } else if (choice >= 1 && choice <= numBaudRates) {
        Serial.println(choice);
        unsigned long newBaud = baudRates[choice - 1];
        
        if (newBaud != currentBaudRate) {
          changeBaudRate(newBaud);
        } else {
          Serial.println();
          Serial.println("Same as current. No change needed.");
          
          lcdClear();
          lcdSetXY(0, 0);
          lcdPrint("No Change");
          lcdSetXY(0, 1);
          lcdPrint("Already using");
          lcdSetXY(0, 2);
          char brBuf[17];
          sprintf(brBuf, "%lu bps", newBaud);
          lcdPrint(brBuf);
          delay(2000);
          
          lcdClear();
          lcdSetXY(0, 0);
          lcdPrint("Tester Ready...");
          
          Serial.println("=====================================");
          Serial.println("Terminal Mode Active!");
          Serial.println("=====================================");
          Serial.println();
        }
        validInput = true;
      } else {
        Serial.println(input);
        Serial.println("Invalid choice! Try again: ");
      }
    }
  }
}

// =====================================================
// CHANGE BAUD RATE
// =====================================================
void changeBaudRate(unsigned long newBaud) {
  Serial.println();
  Serial.println("=====================================");
  Serial.print("Changing Baud Rate from ");
  Serial.print(currentBaudRate);
  Serial.print(" to ");
  Serial.println(newBaud);
  Serial.println("Saving to EEPROM...");
  
  // Simpan ke EEPROM
  saveBaudRateToEEPROM(newBaud);
  
  Serial.println("Saved successfully!");
  Serial.println();
  Serial.println("IMPORTANT:");
  Serial.println("Close and reopen Serial Monitor");
  Serial.print("with new baud rate: ");
  Serial.println(newBaud);
  Serial.println("=====================================");
  Serial.println();
  Serial.println("Switching in 3 seconds...");
  
  // Tampilkan di LCD
  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("Saving BR:");
  lcdSetXY(0, 1);
  char brBuf[17];
  sprintf(brBuf, "%lu", newBaud);
  lcdPrint(brBuf);
  lcdSetXY(0, 2);
  lcdPrint("SAVED TO EEPROM");
  
  delay(2000);
  
  lcdSetXY(0, 3);
  lcdPrint("Switching...");
  
  delay(1000);
  
  // Ganti baud rate
  currentBaudRate = newBaud;
  Serial.end();
  delay(100);
  Serial.begin(newBaud);
  
  // Clear buffer
  for (int i = 0; i < 4; i++) lineBuffer[i] = "";
  
  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("Baud Rate:");
  lcdSetXY(0, 1);
  lcdPrint(brBuf);
  lcdSetXY(0, 2);
  lcdPrint("Reconnect Serial");
  lcdSetXY(0, 3);
  lcdPrint("Monitor now!");
  
  delay(2000);
  
  lcdClear();
  lcdSetXY(0, 0);
  lcdPrint("Ready!");
  
  Serial.println();
  Serial.println("=====================================");
  Serial.println("  BAUD RATE CHANGED SUCCESSFULLY!");
  Serial.println("=====================================");
  Serial.print("New Baud Rate: ");
  Serial.println(currentBaudRate);
  Serial.println();
  Serial.println("Terminal Mode Active!");
  Serial.println("Commands: CLEAR, RESET, MENU, BAUD:xxxxx");
  Serial.println("=====================================");
  Serial.println();
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  // ================= PIN COMMAND =================
  if (digitalRead(PIN_CLEAR) == LOW) {
    lcdClear();
    for (int i = 0; i < 4; i++) lineBuffer[i] = "";
    Serial.println("[LCD CLEARED via button]");
    delay(300);
  }

  if (digitalRead(PIN_RESET) == LOW) {
    lineNumber = 1;
    Serial.println("[Line number RESET via button]");
    delay(300);
  }

  // ================= SERIAL COMMAND =================
  if (Serial.available()) {

    String incoming = Serial.readStringUntil('\n');
    incoming.trim();

    if (incoming.equalsIgnoreCase("CLEAR")) {
      lcdClear();
      for (int i = 0; i < 4; i++) lineBuffer[i] = "";
      Serial.println("[LCD CLEARED]");
      return;
    }

    if (incoming.equalsIgnoreCase("RESET")) {
      lineNumber = 1;
      Serial.println("[Line number RESET to 1]");
      return;
    }

    if (incoming.equalsIgnoreCase("MENU")) {
      showBaudRateMenu();
      return;
    }

    // ================= COMMAND BAUD RATE =================
    if (incoming.startsWith("BAUD:") || incoming.startsWith("baud:")) {
      String baudStr = incoming.substring(5);
      baudStr.trim();
      unsigned long newBaud = baudStr.toInt();
      
      // Validasi baud rate yang umum digunakan
      if (isValidBaudRate(newBaud)) {
        
        if (newBaud == currentBaudRate) {
          Serial.println();
          Serial.print("Already using ");
          Serial.print(newBaud);
          Serial.println(" bps. No change needed.");
          
          lcdClear();
          lcdSetXY(0, 0);
          lcdPrint("No Change");
          lcdSetXY(0, 1);
          lcdPrint("Already using");
          lcdSetXY(0, 2);
          char brBuf[17];
          sprintf(brBuf, "%lu bps", newBaud);
          lcdPrint(brBuf);
          delay(2000);
          lcdClear();
          for (int row = 0; row < 4; row++) {
            lcdSetXY(0, row);
            lcdPrint(lineBuffer[row].c_str());
          }
        } else {
          changeBaudRate(newBaud);
        }
      } else {
        // Baud rate tidak valid
        Serial.println();
        Serial.println("ERROR: Invalid Baud Rate!");
        Serial.println("Valid options: 300, 1200, 2400, 4800, 9600,");
        Serial.println("               14400, 19200, 28800, 38400,");
        Serial.println("               57600, 115200");
        Serial.println();
        
        lcdClear();
        lcdSetXY(0, 0);
        lcdPrint("Invalid BR!");
        lcdSetXY(0, 1);
        lcdPrint("Valid: 9600,");
        lcdSetXY(0, 2);
        lcdPrint("115200, etc");
        lcdSetXY(0, 3);
        lcdPrint("Send 'MENU'");
        delay(3000);
        lcdClear();
        for (int row = 0; row < 4; row++) {
          lcdSetXY(0, row);
          lcdPrint(lineBuffer[row].c_str());
        }
      }
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

// =====================================================
// EEPROM FUNCTIONS
// =====================================================
void saveBaudRateToEEPROM(unsigned long baud) {
  // Simpan baud rate (4 bytes)
  EEPROM.write(EEPROM_ADDR_BAUD, (baud >> 24) & 0xFF);
  EEPROM.write(EEPROM_ADDR_BAUD + 1, (baud >> 16) & 0xFF);
  EEPROM.write(EEPROM_ADDR_BAUD + 2, (baud >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_BAUD + 3, baud & 0xFF);
  
  // Simpan tanda valid
  EEPROM.write(EEPROM_ADDR_VALID, 0xAA);
}

unsigned long loadBaudRateFromEEPROM() {
  // Cek apakah data valid
  if (EEPROM.read(EEPROM_ADDR_VALID) != 0xAA) {
    return 0; // Tidak ada data valid, gunakan default
  }
  
  // Baca baud rate (4 bytes)
  unsigned long baud = 0;
  baud |= ((unsigned long)EEPROM.read(EEPROM_ADDR_BAUD)) << 24;
  baud |= ((unsigned long)EEPROM.read(EEPROM_ADDR_BAUD + 1)) << 16;
  baud |= ((unsigned long)EEPROM.read(EEPROM_ADDR_BAUD + 2)) << 8;
  baud |= EEPROM.read(EEPROM_ADDR_BAUD + 3);
  
  // Validasi apakah baud rate masuk akal
  if (isValidBaudRate(baud)) {
    return baud;
  }
  
  return 0; // Data tidak valid
}

bool isValidBaudRate(unsigned long baud) {
  return (baud == 300 || baud == 1200 || baud == 2400 || 
          baud == 4800 || baud == 9600 || baud == 14400 ||
          baud == 19200 || baud == 28800 || baud == 38400 ||
          baud == 57600 || baud == 115200);
}
