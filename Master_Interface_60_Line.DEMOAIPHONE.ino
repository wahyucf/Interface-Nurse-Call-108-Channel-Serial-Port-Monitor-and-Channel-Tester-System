//Ceated by WahyuCF 2022

#include <Wire.h>

/*
   DEMO AIPHONE
   Rubah jumlah line 12 LINE

   Serial2 TX=PIN16 RX=PIN17

*/

/*
  Rubah Tegangan iPhone/Commax
  iPhone: 150 - tanpa R 100K ohm
  Commax: 50 - dengan R 100K ohm
*/
//============================================
float LimitVal = 70;// Set tegangan minimun
//============================================


/*
  Rubah Jumlah line dibawah ini
*/
//============================================
int JmlLine = 48   ;// Set jumlah line
//============================================


//===================================
int check = 0;
//===================================


int myInts[54];
char str[54];

//============================================
int TikTime = 10;
int DispTime = 12;
//============================================
unsigned long millisA = 0;
unsigned long millisB = 0;
unsigned long millisC = 0;
unsigned long millisD = 0;
int lineOn = 0;
int lineOn1 = 0;
int lineNum [10];
//============================================

int myPins[] =              {A0, A1, A2, A3, A4, A5, A6}; //, A7, A8, A9, A10, A11
float mySensVals[] =        {
  1, 1, 1, 1, 1, 1, 1, 1,  //08
  1, 1, 1, 1, 1, 1, 1, 1,  //16
  1, 1, 1, 1, 1, 1, 1, 1,  //24
  1, 1, 1, 1, 1, 1, 1, 1,  //32
  1, 1, 1, 1, 1, 1, 1, 1,  //40
  1, 1, 1, 1, 1, 1, 1, 1,  //48
  1, 1, 1, 1, 1, 1, 1, 1,  //56
  1, 1, 1, 1, 1, 1, 1, 1,  //64
  1, 1, 1, 1, 1, 1, 1      //71
};
float mySensValsOld[] =     {
  1, 1, 1, 1, 1, 1, 1, 1,  //08
  1, 1, 1, 1, 1, 1, 1, 1,  //16
  1, 1, 1, 1, 1, 1, 1, 1,  //24
  1, 1, 1, 1, 1, 1, 1, 1,  //32
  1, 1, 1, 1, 1, 1, 1, 1,  //40
  1, 1, 1, 1, 1, 1, 1, 1,  //48
  1, 1, 1, 1, 1, 1, 1, 1,  //56
  1, 1, 1, 1, 1, 1, 1, 1,  //64
  1, 1, 1, 1, 1, 1, 1      //71
};
int mySensValsStat[] =      {
  1, 1, 1, 1, 1, 1, 1, 1,  //08
  1, 1, 1, 1, 1, 1, 1, 1,  //16
  1, 1, 1, 1, 1, 1, 1, 1,  //24
  1, 1, 1, 1, 1, 1, 1, 1,  //32
  1, 1, 1, 1, 1, 1, 1, 1,  //40
  1, 1, 1, 1, 1, 1, 1, 1,  //48
  1, 1, 1, 1, 1, 1, 1, 1,  //56
  1, 1, 1, 1, 1, 1, 1, 1,  //64
  1, 1, 1, 1, 1, 1, 1      //71  
};


/*
   Rubah Nomor Kamar dibawah ini
   tanda: "< dan >" harus tetap ada

   PIN-1 untuk PIN/Line 1
   PIN-2 untuk PIN/Line 2
   dst

*/

String KirimDisply [] =
{
  //=================================
  // ---> Awal list nomor kamar
  //=================================

  "<N>",          //PIN-0
  "<RR101-01>",   //PIN-1
  "<KKM-101>",     //PIN-2
  "<RR102-01>",      //PIN-3
  "<KKM-102>",       //PIN-4
  "<RR103-01>",      //PIN-5
  "<KKM-103>",      //PIN-6
  "<RR104-01>",      //PIN-7
  "<KKM-104>",      //PIN-8
  "<RR105-01>",      //PIN-9
  "<KKM-105>",      //PIN-10
  "<RR106-01>",      //PIN-11
  "<KKM-106>",      //PIN-12
  "<RR107-01>",      //PIN-13
  "<KKM-107>",      //PIN-14
  "<RR108-01>",      //PIN-15
  "<KKM-108>",      //PIN-16
  "<RR109-01>",      //PIN-17
  "<KKM-109>",      //PIN-18
  "<RR110-01>",      //PIN-19
  "<KKM-110>",      //PIN-20
  "<RR111-01>",      //PIN-21
  "<KKM-111>",      //PIN-22
  "<RR112-01>",      //PIN-23
  "<KKM-112>",      //PIN-24
  "<RR113-01>",      //PIN-25
  "<KKM-113>",      //PIN-26
  "<RR114-01>",      //PIN-27
  "<KKM-114>",      //PIN-28
  "<RR115-01>",      //PIN-29
  "<KKM-115>",      //PIN-30
  "<RR116-01>",      //PIN-31
  "<KKM-116>",      //PIN-32
  "<RR117-01>",      //PIN-33
  "<KKM-117>",      //PIN-34
  "<RR118-01>",      //PIN-35
  "<KKM-118>",      //PIN-36
  "<RR119-01>",      //PIN-37
  "<KKM-119>",      //PIN-38
  "<RR120-01>",      //PIN-39
  "<KKM-120>",      //PIN-40
  "<RR121-01>",      //PIN-41
  "<KKM-121>",      //PIN-42
  "<RR122-01>",      //PIN-43
  "<KKM-122>",      //PIN-44
  "<RR123-01>",      //PIN-45
  "<KKM-123>",      //PIN-46
  "<RR124-01>",      //PIN-47
  "<KKM-124>",      //PIN-48
  "<RR125-01>",      //PIN-49
  "<KKM-125>",      //PIN-50
  "<RR126-01>",      //PIN-51
  "<KKM-126>",      //PIN-52
  "<RR127-01>",      //PIN-53
  "<KKM-127>",      //PIN-54
  "<RR.M1-01>",      //PIN-55
  "<RR.M1-02>",      //PIN-56
  "<RR.M1-03>",      //PIN-57
  "<RR.M1-04>",      //PIN-58
  "<KKM-M1>",      //PIN-59
  "<CCB-NOL>"      //PIN-60
  "<RR61>",      //PIN-61
  "<KKM62>",      //PIN-62
  "<RR63>",      //PIN-63
  "<KKM64>",      //PIN-64
  "<RR65>",      //PIN-65
  "<RR66>",      //PIN-66
  "<RR67>",      //PIN-67
  "<RR68>",      //PIN-68
  "<KKM69>",      //PIN-69
  "<CCB70>"      //PIN-70


  //=================================
  // <--- Akhir list nomor kamar
  //=================================

};


// Mlk01
//int mySensValsMap[] =      {49,50,51,52,53,54,55,56,57,58,59,60,37,38,39,40,45,42,43,44,41,46,47,48,25,28,27,26,29,30,31,32,33,34,35,36,13,14,15,16,17,18,19,20,21,22,23,24,1,2,3,4,5,6,7,8,9,10,11,12,61,62,63,64,65,66};
//=========================================================================================================================================================================================================================================


int printdisp = 0;
int count = 0;
int count1 = 0;
int count2 = 0;
int count3 = 0;
int count4 = 0;
int count5 = 0;
int timer = 100;
int timer1 = 900;
int timer2 = 100;
int a = 0;
//int id;
int idS;
int idS1 = 0;

bool valTot = HIGH;      // variable to store the read value
int value;
unsigned long interval = 500; // the time we need to wait
unsigned long previousMillis = 0; // millis() returns an unsigned long.


int pin_Out_S0 = 22;
int pin_Out_S1 = 23;
int pin_Out_S2 = 24;
int pin_Out_S3 = 25;

int pin_Out_S4 = 26;
int pin_Out_S5 = 27;
int pin_Out_S6 = 28;
int pin_Out_S7 = 29;

int pin_Out_S8 = 30;
int pin_Out_S9 = 31;
int pin_Out_S10 = 32;
int pin_Out_S11 = 33;

int pin_Out_S12 = 38;
int pin_Out_S13 = 39;
int pin_Out_S14 = 40;
int pin_Out_S15 = 41;


int pin_In_Mux1 = A15;
int Mux1_State[16] = {0};

int pin_In_Mux2 = A14;
int Mux2_State[16] = {0};

int pin_In_Mux3 = A13;
int Mux3_State[16] = {0};

int pin_In_Mux4 = A12;
int Mux4_State[16] = {0};



void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial.println("System Started");

  //===============================================
  pinMode(pin_Out_S0, OUTPUT);
  pinMode(pin_Out_S1, OUTPUT);
  pinMode(pin_Out_S2, OUTPUT);
  pinMode(pin_Out_S3, OUTPUT);
  //===============================================
  pinMode(pin_Out_S4, OUTPUT);
  pinMode(pin_Out_S5, OUTPUT);
  pinMode(pin_Out_S6, OUTPUT);
  pinMode(pin_Out_S7, OUTPUT);
  //===============================================
  pinMode(pin_Out_S8, OUTPUT);
  pinMode(pin_Out_S9, OUTPUT);
  pinMode(pin_Out_S10, OUTPUT);
  pinMode(pin_Out_S11, OUTPUT);
  //===============================================
  pinMode(pin_Out_S12, OUTPUT);
  pinMode(pin_Out_S13, OUTPUT);
  pinMode(pin_Out_S14, OUTPUT);
  pinMode(pin_Out_S15, OUTPUT);
  //===============================================

  delay(1000);
  for (count = 0; count < 16; count++) {
    pinMode(myPins[count], INPUT);
    //digitalWrite(myPins[count], HIGH);
  }
  pinMode(13, OUTPUT);    // sets the digital pin 13 as output
  digitalWrite(13, LOW);  // sets the digital pin 13 off

  CodeOn();
  Serial.println("System Ready");
  Serial2.print("<N>");
}

void CodeOn() {
  digitalWrite(13, HIGH);
  delay(200);
  digitalWrite(13 , LOW);
}

void CodeOn1() {
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13 , LOW);
  delay(200);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13 , LOW);
}


void loop()
{

  idS1 = 0;
  valTot = HIGH;

  //if (valTot == LOW){

  for (count = 0; count < JmlLine; count++)  {
    printdisp = count + 1;
    if (count < 16) {
      digitalWrite(pin_Out_S0, HIGH && (count & B00000001));
      digitalWrite(pin_Out_S1, HIGH && (count & B00000010));
      digitalWrite(pin_Out_S2, HIGH && (count & B00000100));
      digitalWrite(pin_Out_S3, HIGH && (count & B00001000));
      value = analogRead(pin_In_Mux1);
      delay(10);

      mySensVals[count] = value;

    } else if (count > 15 && count < 32 ) {
      count1 = count - 16;
      digitalWrite(pin_Out_S4, HIGH && (count1 & B00000001));
      digitalWrite(pin_Out_S5, HIGH && (count1 & B00000010));
      digitalWrite(pin_Out_S6, HIGH && (count1 & B00000100));
      digitalWrite(pin_Out_S7, HIGH && (count1 & B00001000));
      value = analogRead(pin_In_Mux2);
      delay(10);

      mySensVals[count] = value;

    } else if (count > 31 && count < 48 ) {
      count2 = count - 32;
      digitalWrite(pin_Out_S8, HIGH && (count2 & B00000001));
      digitalWrite(pin_Out_S9, HIGH && (count2 & B00000010));
      digitalWrite(pin_Out_S10, HIGH && (count2 & B00000100));
      digitalWrite(pin_Out_S11, HIGH && (count2 & B00001000));
      value = analogRead(pin_In_Mux3);
      delay(10);

      mySensVals[count] = value;

    } else if (count > 47 && count < 64 ) {
      count3 = count - 48;
      digitalWrite(pin_Out_S12, HIGH && (count3 & B00000001));
      digitalWrite(pin_Out_S13, HIGH && (count3 & B00000010));
      digitalWrite(pin_Out_S14, HIGH && (count3 & B00000100));
      digitalWrite(pin_Out_S15, HIGH && (count3 & B00001000));
      value = analogRead(pin_In_Mux4);
      delay(10);

      mySensVals[count] = value;

    } else {
      count5 = count - 65;
      int value = analogRead(myPins[count5]);
      delay(5);
      mySensVals[count] = value;
      delay(5);
    }


    //=============================
    if ((mySensVals[count] > 0) && (check == 1)) {
      Serial2.print("A ");
      Serial2.print(printdisp);
      Serial.print(": ");
      Serial.print(mySensVals[count]);
      Serial.println("/1023");
    }
    //=============================

    if ((mySensVals[count] < LimitVal)) {
      if ((mySensValsStat[count] == 0)) {
        Serial.print("90");
        Serial.print(printdisp);
        Serial.println(":");
        Serial2.print("<N>");
        mySensValsStat[count] = 1;
        lineOn = 0;
        millisC = 0;
        delay(10);
      }
      continue;
    }

    //=============================
    if ((mySensVals[count] > 0) && (check == 1)) {
      Serial2.print("B ");
      Serial2.print(printdisp);
      Serial.print(": ");
      Serial.print(mySensVals[count]);
      Serial.println("/1023");
    }
    //=============================


    {
      //if ((mySensValsStat[count] == 1))
      {
        mySensValsOld[count] = mySensVals[count];
        //printdisp = mySensValsMap[count];

        //=============================
        if ((mySensVals[count] > 0) && (check == 1)) {
          Serial.print("millisC: ");
          Serial.print(millisC);
          Serial.print(" : ");
          Serial.print(DispTime);
          Serial.println("");
        }
        //=============================


        if ((millisC >= DispTime) || (millisC == 0)) {

          //=============================
          if ((mySensVals[count] > 0) && (check == 1)) {
            Serial.print("millisC: ");
            Serial.print(millisC);
            Serial.print(" : ");
            Serial.print(DispTime);
            Serial.println("");
          }
          //=============================


          Serial.print("10");
          Serial.print(printdisp);
          Serial.print(": ");
          Serial.println(mySensVals[count]);
          //Serial.println(KirimDisply [printdisp]);
          Serial2.print(KirimDisply [printdisp]);
          //Kir_Dat (printdisp);
          delay(10);
          millisC = 0;
          CodeOn1();
        }
        millisC = millisC + 1;
        //delay(7500);
        mySensValsStat[count]  =  0;
        lineOn = 1;
      }
    }
  }
  count = 0;
  //}
  CodeOn();


  //if (valTot == HIGH){


  millisB = millisB + 1;
  if ((millisB >= TikTime) && (lineOn <= 0)) {
    Serial.println("99:");
    Serial2.print("<N>");
    delay(5);
    millisB = 0;
  }



  //}

}
