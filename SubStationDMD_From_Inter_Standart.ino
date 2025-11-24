/*
  Countdown on a single DMD display
*/

#include <SPI.h>
//#include <SoftwareSerial.h>
#include <DMD2.h>
#include <fonts/Arial_Black_16.h>
#include <fonts/Droid_Sans_12.h>
#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 2
const int COUNTDOWN_FROM = 12;
int counter = COUNTDOWN_FROM;
uint16_t lastPrint = 100;
String state;
boolean ret = false;
int NoQ = 0;
int id;
int millisB = 0;
int Pin13 = 0;

unsigned long interval = 10000; // the time we need to wait
unsigned long previousMillis = 0; // millis() returns an unsigned long.




byte x;
char myArray[20] = "";
int i = 0;
bool flag1 = LOW;
String IsiBerita;
String tipeBeep;
int j = 0;
String pesanDisplay;


//SoftwareSerial mySerial1(2, 3); // RX, TX
SoftDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN); // DMD controls the entire display
DMD_TextBox box(dmd, 0, 0); // "box" provides a text box to automatically write to/scroll the display

// the setup routine runs once when you press reset:

void drawText(String dispString)
{
  dmd.clearScreen();
  int sLength = dispString.length();
  dmd.drawString(1, 1, dispString);
  dmd.drawString(1, 17, dispString);
}

void beepCB()
{
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  delay(100);
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  delay(100);
  //digitalWrite(4, HIGH);
  //delay(100);
  //digitalWrite(4, LOW);
}


void beepSA()
{
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(4, LOW);
  delay(100);
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(4, LOW);
  delay(100);
  //digitalWrite(4, HIGH);
  //delay(500);
  //digitalWrite(4, LOW);
  //delay(100);
}

void beeppendek()
{
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
}
void beeppanjang()
{
  digitalWrite(4, HIGH);
  delay(300);
  digitalWrite(4, LOW);
}

void DispBed(String a)
{
  String b = a;//+"B";
  //        Serial.print ("Tampil: ");
  //        Serial.println (b);
  drawText(b);
  beeppendek();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppendek();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppendek();
  //delay(900);
  //dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beeppendek();
  delay(1000);
  dmd.clearScreen();

  //         Serial.println("0:");
  //         delay(50);

}
void DispRBed(String a, String c)
{
  String b = a;//+" B"+c;
  //        Serial.print ("Tampil: ");
  //        Serial.println (b);
  drawText(b);
  beeppendek();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppendek();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppendek();
  //delay(900);
  //dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beeppendek();
  delay(1000);
  dmd.clearScreen();

  //          Serial.println("0:");

}
void DispKM(String a)
{
  String b = a;//+":KM";
  //        Serial.print ("Tampil: ");
  //        Serial.println (b);
  drawText(b);
  beeppanjang();
  delay(500);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppanjang();
  delay(500);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beeppanjang();
  //delay(500);
  //dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beeppanjang();
  delay(1000);
  dmd.clearScreen();

  //          Serial.println("0:");

}

void DispCBlue(String a)
{
  String b = a;// +" CB";
  digitalWrite(10, HIGH);
  //        Serial.print ("Tampil: ");
  //        Serial.println (b);
  drawText(b);
  beepCB();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beepCB();
  delay(500);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beepCB();
  //delay(900);
  //dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beepCB();
  delay(1000);
  dmd.clearScreen();

  //         Serial.println("0:");
  //          delay(200);
  digitalWrite(10, LOW);

}

void DispSA(String a)
{
  String b = a; //+" SA";
  digitalWrite(10, HIGH);
  //        Serial.print ("Tampil: ");
  //        Serial.println (b);
  drawText(b);
  beepSA();
  delay(900);
  dmd.clearScreen();
  delay(500);
  drawText(b);
  beepSA();
  delay(500);
  dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beepSA();
  //delay(900);
  //dmd.clearScreen();
  //delay(500);
  //drawText(b);
  //beepSA();
  delay(1000);
  dmd.clearScreen();

  //         Serial.println("0:");
  //          delay(200);
  digitalWrite(10, LOW);

}

int phase = 0;
void DotFly() {
  dmd.clearScreen();
  dmd.drawString(0, 0, F("."));
  int steps = random(64); // Each time we scroll a random distance
  for (int i = 0; i < steps; i++) {
    dmd.drawString(i, 0, F("."));
    delay(50);
  }

  // Move to the next phase
  phase = (phase + 1) % 4;
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

void setup() {
  Serial.begin(9600);
  //    Serial2.begin(9600);
  //    mySerial1.begin( 9600 ); // baud-rate of RS485

  pinMode (10, OUTPUT);
  digitalWrite(10, HIGH);
  dmd.setBrightness(50);
  dmd.selectFont(Arial_Black_16);
  dmd.begin();
  dmd.clearScreen();
  drawText(" Nurse ");
  pinMode(4, OUTPUT);    // sets the digital pin 4 as output
  digitalWrite(4, HIGH);
  delay(200);
  digitalWrite(10, HIGH);
  digitalWrite(4, LOW);
  delay( 400 );
  digitalWrite(10, LOW);
  digitalWrite(4, HIGH);
  delay(200);
  digitalWrite(10, HIGH);
  digitalWrite(4, LOW);
  delay( 400 );
  digitalWrite(10, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(10, HIGH);
  delay(1000);
  digitalWrite(4, LOW);
  delay( 400 );
  digitalWrite(10, LOW);
  digitalWrite(4, HIGH);
  delay(200);
  digitalWrite(10, HIGH);
  digitalWrite(4, LOW);
  delay( 400 );
  digitalWrite(10, LOW);
  delay(500);
  dmd.clearScreen();
  Serial.println ("Sub Start");
  digitalWrite(10, LOW);

  id = 100;
  pinMode(23, OUTPUT); //Rubah sesuai PIN lampu indikator (1 dari 3)
}

void CodeOn() {
  digitalWrite(23, HIGH);
  delay(100);
  digitalWrite(23 , LOW);
  delay(100);
}

// the loop routine runs over and over again forever:
void loop() {

  byte b;

  if (Serial.available() > 0)
  {
    if (flag1 == HIGH) //< received
    {
      x = Serial.read();
      if (x != '>')
      {
        myArray[i] = x;
        i++;
        if (i == 1 ) {
          tipeBeep = myArray;

          if (tipeBeep == "N") id = 0; // Blinking
          if (tipeBeep == "R") id = 1; // Room/Kamar
          if (tipeBeep == "K") id = 2; // Kamar Mandi
          if (tipeBeep == "C") id = 3; // Code Blue
          if (tipeBeep == "S") id = 4; // Staff Assist
          if (tipeBeep == "P") id = 5; // Staff Present
          //Serial.println("Type Beep is:");
          //Serial.print(tipeBeep);
          //Serial.print(" - id:");
          //Serial.println(id);
        }
      }
      else
      {
        //Serial.println();
        //Serial.println("Input is:");
        //Serial.println(myArray);
        {
          IsiBerita = myArray;
          IsiBerita = IsiBerita.substring(1);
          //Serial.println("Data is:");
          //Serial.println(IsiBerita);
        }
        flag1 = LOW;
        memset(myArray, 0, sizeof(myArray));
        i = 0;
        j++;
      }
    }
    else
    {
      if (Serial.read() == '<')
      {
        flag1 = HIGH;
      }
    }

    if (flag1 == LOW) //< received
    {

      switch (id) {
        case 1:
          DispBed(IsiBerita);
          //Serial.println("Display Bed is:");
          //Serial.println(IsiBerita);

          Serial.begin(9600); break;
        case 2:
          DispKM(IsiBerita);
          //Serial.println("Display KM is:");
          //Serial.println(IsiBerita);
          Serial.begin(9600); break;
        case 3:
          DispCBlue(IsiBerita);
          //Serial.println("Display CB is:");
          //Serial.println(IsiBerita);
          Serial.begin(9600); break;
        case 4:
          DispSA(IsiBerita);
          //Serial.println("Display SA is:");
          //Serial.println(IsiBerita);
          Serial.begin(9600); break;
        case 5:
          DispBed(IsiBerita);
          Serial.begin(9600); break;

        case 0:
          StatCon();
          id = 100;
          Serial.begin(9600); break;
        default :
          digitalWrite(10, LOW);
      }
    }




  }
  if ( j == 2) {
    j = 0;
  }





  //mySerial1.flush();

  digitalWrite(10, LOW);
  //RunRuang = 0;
  millisB = millisB + 1;
  if (millisB >= 30000) {
  if (Pin13 == 0) {
      digitalWrite(23, HIGH); //Rubah sesuai PIN lampu indikator (2 dari 3)
      millisB = 0;
      Pin13 = 1;
    } else {
      digitalWrite(23 , LOW); //Rubah sesuai PIN lampu indikator (3 dari 3)
      millisB = 0;
      Pin13 = 0;
    }
  }

}
