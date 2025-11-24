/*
  Robust 24/7 DMD Nurse Call Display System
  - Watchdog timer protection
  - Memory leak prevention
  - Overflow protection
  - Error recovery
*/

#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial_Black_16.h>
#include <fonts/Droid_Sans_12.h>
#include <avr/wdt.h>  // Watchdog timer

#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 2
#define SERIAL_TIMEOUT 5000
#define MAX_MESSAGE_LENGTH 20
#define HEARTBEAT_INTERVAL 30000

// Variabel global dengan proteksi overflow
int id = 100;
unsigned long millisB = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSerialActivity = 0;
int Pin13 = 0;
uint8_t i = 0;
uint8_t j = 0;
bool flag1 = LOW;
char myArray[MAX_MESSAGE_LENGTH] = "";
String IsiBerita;
String tipeBeep;

SoftDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
DMD_TextBox box(dmd, 0, 0);

// Fungsi untuk reset watchdog timer
void resetWatchdog() {
  wdt_reset();
}

// Fungsi untuk inisialisasi ulang display jika error
void reinitDisplay() {
  dmd.clearScreen();
  delay(100);
  dmd.begin();
  dmd.setBrightness(50);
  dmd.selectFont(Arial_Black_16);
}

void drawText(String dispString) {
  if (dispString.length() == 0) return;
  
  dmd.clearScreen();
  resetWatchdog();
  
  // Batasi panjang string untuk menghindari overflow
  if (dispString.length() > 15) {
    dispString = dispString.substring(0, 15);
  }
  
  dmd.drawString(1, 1, dispString);
  dmd.drawString(1, 17, dispString);
}

void beepCB() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(4, HIGH);
    delay(100);
    resetWatchdog();
    digitalWrite(4, LOW);
    delay(100);
  }
}

void beepSA() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(4, HIGH);
    delay(500);
    resetWatchdog();
    digitalWrite(4, LOW);
    delay(100);
  }
}

void beeppendek() {
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
}

void beeppanjang() {
  digitalWrite(4, HIGH);
  delay(300);
  digitalWrite(4, LOW);
}

void DispBed(String a) {
  if (a.length() == 0) return;
  
  for (int i = 0; i < 3; i++) {
    drawText(a);
    beeppendek();
    delay(900);
    resetWatchdog();
    dmd.clearScreen();
    delay(500);
  }
  drawText(a);
  beeppendek();
  delay(1000);
  resetWatchdog();
  dmd.clearScreen();
}

void DispRBed(String a, String c) {
  DispBed(a);
}

void DispKM(String a) {
  if (a.length() == 0) return;
  
  for (int i = 0; i < 3; i++) {
    drawText(a);
    beeppanjang();
    delay(500);
    resetWatchdog();
    dmd.clearScreen();
    delay(500);
  }
  drawText(a);
  beeppanjang();
  delay(1000);
  resetWatchdog();
  dmd.clearScreen();
}

void DispCBlue(String a) {
  if (a.length() == 0) return;
  
  digitalWrite(10, HIGH);
  for (int i = 0; i < 3; i++) {
    drawText(a);
    beepCB();
    delay(900);
    resetWatchdog();
    dmd.clearScreen();
    delay(500);
  }
  drawText(a);
  beepCB();
  delay(1000);
  resetWatchdog();
  dmd.clearScreen();
  digitalWrite(10, LOW);
}

void DispSA(String a) {
  if (a.length() == 0) return;
  
  digitalWrite(10, HIGH);
  for (int i = 0; i < 2; i++) {
    drawText(a);
    beepSA();
    delay(900);
    resetWatchdog();
    dmd.clearScreen();
    delay(500);
  }
  delay(1000);
  resetWatchdog();
  dmd.clearScreen();
  digitalWrite(10, LOW);
}

void StatCon() {
  dmd.selectFont(Droid_Sans_12);
  digitalWrite(10, HIGH);
  drawText(".");
  delay(500);
  digitalWrite(10, LOW);
  dmd.clearScreen();
  delay(100);
  id = 100;
  dmd.selectFont(Arial_Black_16);
}

// Fungsi untuk membersihkan buffer serial
void clearSerialBuffer() {
  while (Serial.available() > 0) {
    Serial.read();
    delay(1);
  }
}

// Fungsi untuk reset state komunikasi
void resetCommState() {
  flag1 = LOW;
  memset(myArray, 0, sizeof(myArray));
  i = 0;
  id = 100;
}

void setup() {
  // Disable watchdog saat startup
  wdt_disable();
  
  Serial.begin(9600);
  Serial.setTimeout(SERIAL_TIMEOUT);
  
  pinMode(10, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(23, OUTPUT);
  
  // Inisialisasi semua output ke LOW
  digitalWrite(10, LOW);
  digitalWrite(4, LOW);
  digitalWrite(23, LOW);
  
  digitalWrite(10, HIGH);
  dmd.setBrightness(50);
  dmd.selectFont(Arial_Black_16);
  dmd.begin();
  dmd.clearScreen();
  drawText(" Nurse ");
  
  // Sequence beep dan LED
  for (int i = 0; i < 2; i++) {
    digitalWrite(4, HIGH);
    digitalWrite(10, i % 2);
    delay(200);
    digitalWrite(4, LOW);
    delay(400);
    digitalWrite(10, LOW);
  }
  
  digitalWrite(4, HIGH);
  digitalWrite(10, HIGH);
  delay(1000);
  digitalWrite(4, LOW);
  delay(400);
  digitalWrite(10, LOW);
  digitalWrite(4, HIGH);
  delay(200);
  digitalWrite(10, HIGH);
  digitalWrite(4, LOW);
  delay(400);
  digitalWrite(10, LOW);
  delay(500);
  
  dmd.clearScreen();
  Serial.println("Sub Start");
  Serial.flush();
  digitalWrite(10, LOW);
  id = 100;
  
  // Inisialisasi timestamp
  lastHeartbeat = millis();
  lastSerialActivity = millis();
  
  // Enable watchdog timer (8 detik)
  wdt_enable(WDTO_8S);
}

void loop() {
  // Reset watchdog di awal loop
  resetWatchdog();
  
  // Cek timeout komunikasi serial
  if (flag1 == HIGH && (millis() - lastSerialActivity > SERIAL_TIMEOUT)) {
    resetCommState();
    clearSerialBuffer();
  }
  
  if (Serial.available() > 0) {
    lastSerialActivity = millis();
    
    if (flag1 == HIGH) {
      byte x = Serial.read();
      
      if (x != '>') {
        // Proteksi buffer overflow
        if (i < (MAX_MESSAGE_LENGTH - 1)) {
          myArray[i] = x;
          i++;
          
          if (i == 1) {
            tipeBeep = String(myArray[0]);
            
            // Gunakan switch untuk performa lebih baik
            switch(myArray[0]) {
              case 'N': id = 0; break;
              case 'R': id = 1; break;
              case 'K': id = 2; break;
              case 'C': id = 3; break;
              case 'S': id = 4; break;
              case 'P': id = 5; break;
              default: id = 100; break;
            }
          }
        } else {
          // Buffer penuh, reset
          resetCommState();
        }
      } else {
        // Akhir pesan
        myArray[i] = '\0'; // Null terminate
        IsiBerita = String(myArray);
        if (IsiBerita.length() > 0) {
          IsiBerita = IsiBerita.substring(1);
        }
        flag1 = LOW;
        memset(myArray, 0, sizeof(myArray));
        i = 0;
        j++;
        
        // Batasi j untuk mencegah overflow
        if (j > 100) j = 0;
      }
    } else {
      if (Serial.read() == '<') {
        flag1 = HIGH;
        i = 0;
        memset(myArray, 0, sizeof(myArray));
      }
    }

    if (flag1 == LOW && id != 100) {
      // Proses perintah
      switch (id) {
        case 0: 
          StatCon(); 
          id = 100; 
          break;
        case 1: 
          DispBed(IsiBerita); 
          id = 100;
          break;
        case 2: 
          DispKM(IsiBerita); 
          id = 100;
          break;
        case 3: 
          DispCBlue(IsiBerita); 
          id = 100;
          break;
        case 4: 
          DispSA(IsiBerita); 
          id = 100;
          break;
        case 5: 
          DispBed(IsiBerita); 
          id = 100;
          break;
        default: 
          digitalWrite(10, LOW); 
          id = 100;
          break;
      }
      
      // Clear data setelah diproses
      IsiBerita = "";
    }
  }
  
  if (j >= 2) {
    j = 0;
  }

  digitalWrite(10, LOW);
  
  // Gunakan millis() yang lebih aman
  unsigned long currentMillis = millis();
  
  // Heartbeat LED indicator dengan millis()
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentMillis;
    
    if (Pin13 == 0) {
      digitalWrite(23, HIGH);
      Pin13 = 1;
    } else {
      digitalWrite(23, LOW);
      Pin13 = 0;
    }
  }
  
  // Small delay untuk stabilitas
  delay(10);
}
