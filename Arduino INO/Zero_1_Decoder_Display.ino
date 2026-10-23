//  Zero 1 Decoder Display for Arduino Mega
//
//  Beta 0.2
//
//  Hornby Zero 1 decoder and display
//
//  Written and Copyright (C) by Justin M Dunlop (Zero1Guy) 2026
//
//  Please do not distrute without written permission from the author
//
//  NB:  Pin configurations below will need to be followed and/or edited to suit your set up
//
//  Refer to https://github.com/justdun/Zero-1-Decoder-Display for further details and instructons
//    and to https://www.youtube.com/@zero1guy for video demonstrations 
//

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

byte Z1_Pin = 3;
byte Z1_Count = 0;
byte Z1_Data[34];
byte Z1_Tag;
int Z1_LocoNo[ ] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
byte Z1_LocoSpeed[ ] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;
byte Z1_LocoDirection[ ] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;
byte j;

unsigned long Z1_PulseDuration;
int AccessoryNo;

volatile uint16_t datapoint[1001];
volatile int savepoint = 1;
int readpoint = 1;
int lastsavepoint = 1;
volatile int chaseflag = 0;

void setup()

{

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.println(F("Welcome to Zero DCC!"));
  Serial.println();
  Serial.println(F("You should only see DCC++EX commands here (or errors if things go wrong)."));
  Serial.println();

  pinMode (digitalPinToInterrupt(Z1_Pin), INPUT);

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x8410
#define Z_DCC   0xBDD7
#define Z_DCC_RED 0xA800

  uint16_t ID;
  ID = tft.readID();
  if (ID == 0x0D3D3) ID = 0x7796;
  tft.begin(ID);

  tft.fillScreen(GRAY);
  tft.setTextColor(BLACK, Z_DCC);
  tft.setTextSize(2);

  Update_Screen_Locos();

  for (int a = 1; a <= 99; a++) {
    Update_Screen_Accessory(a, 2);
  }

  Track_Power_Off();

  attachInterrupt (digitalPinToInterrupt(Z1_Pin), Z1_CALC_PULSE, CHANGE);

}

void loop()
{

  interrupts();
  Data_Cycle();

}

void Z1_CALC_PULSE()
{

  if ( digitalRead( Z1_Pin ) == HIGH )
  {

    datapoint[savepoint] = micros();

    if (savepoint == 1000) {
      savepoint = 1;
      chaseflag = 1;
    }
    else {
      savepoint = savepoint + 1;
    }
  }
}

void Data_Cycle()
{

  int lastchaseflag;
  uint16_t thisPulse;
  uint16_t nextPulse;

  noInterrupts();
  lastsavepoint = savepoint;
  lastchaseflag = chaseflag;
  interrupts();

  if (((lastsavepoint > (readpoint + 2) ) && (lastchaseflag == 0))  || ((lastsavepoint < readpoint ) && (lastchaseflag == 1))) {

    noInterrupts();
    thisPulse = datapoint[readpoint];
    if (readpoint == 1000) {
      nextPulse = datapoint[1];
    }
    else {
      nextPulse = datapoint[readpoint + 1];
    }
    interrupts();

    Z1_PulseDuration = (uint16_t)(nextPulse - thisPulse);

    if (readpoint == 1000) {
      readpoint = 1;
      noInterrupts();
      chaseflag = 0;
      interrupts();
    }
    else  {
      readpoint = readpoint + 1;
    }

    delay(1);

    if (Z1_PulseDuration > 1000) {
      if (Z1_Count == 33) {
        decoder();
      }
      Z1_Count = 0;
      }

    else if (Z1_PulseDuration > 10 && Z1_PulseDuration < 150) {
      if (Z1_Count < 33) {
        Z1_Count = Z1_Count + 1;
        Z1_Data[Z1_Count] = 0;
      }
      }

    else if (Z1_PulseDuration > 160 && Z1_PulseDuration < 250) {
      if (Z1_Count < 33) {
        Z1_Count = Z1_Count + 1;
        Z1_Data[Z1_Count] = 1;
      }
      }

  }

}

void decoder() {

  if ((Z1_Data[8] + Z1_Data[7] + Z1_Data[6] + Z1_Data[5]) == 0) {

    AccessoryNo = (((Z1_Data[13] * 1) + (Z1_Data[14] * 2) + (Z1_Data[15] * 4) + (Z1_Data[16] * 8)) * 10) + ((Z1_Data[17] * 1) + (Z1_Data[18] * 2) + (Z1_Data[19] * 4) + (Z1_Data[20] * 8));

    Point_Routine();
  }

  else {
    Locomotive_Routine();
  }

  Z1_Count = 0;

}

void Locomotive_Routine() {

  Z1_Tag = (Z1_Data[3] * 1) + (Z1_Data[4] * 2);

  int LocoSpeed[5];
  int LocoDirection[5];
  j = 1;
  for (j = 1 ; j <= 4 ; j++) {
    int Change = 0;

    LocoSpeed[j] = ((Z1_Data[1 + (j * 4)] * 1) + (Z1_Data[2 + (j * 4)] * 2) + (Z1_Data[3 + (j * 4)] * 4) + (Z1_Data[4 + (j * 4)] * 8) - 1);

    LocoSpeed[j] = map(LocoSpeed[j], 0, 14, 0, 126);

    if (Z1_LocoSpeed [j + (Z1_Tag * 4)] != LocoSpeed[j] )  {
      Change = 1;
      Z1_LocoSpeed[j + (Z1_Tag * 4)] = LocoSpeed[j];
    }

    LocoDirection[j] = Z1_Data [20 + j];

    if (Z1_LocoDirection[j + (Z1_Tag * 4)] != LocoDirection[j]) {
      Change = 1;
      Z1_LocoDirection[j + (Z1_Tag * 4)] = LocoDirection[j];
    }

    if (Change == 1) {
      Serial.print("<t ");
      Serial.print(j + (Z1_Tag * 4));
      Serial.print(" ");
      Serial.print(Z1_LocoNo[j + (Z1_Tag * 4)]);
      Serial.print(" ");
      Serial.print(Z1_LocoSpeed[j + (Z1_Tag * 4)]);
      Serial.print(" ");
      Serial.print(Z1_LocoDirection[j + (Z1_Tag * 4)]);
      Serial.println(">");

      Update_Screen_Speeds(int (j + (Z1_Tag * 4)));
      Change = 0;
    }
  }

}

void Point_Routine() {

  if (Z1_Data[24] == 1) {
    Serial.print("<T ");
    Serial.print(AccessoryNo);
    Serial.println(" 1>");
    Update_Screen_Accessory(AccessoryNo, 1);
  }

  else {
    Serial.print("<T ");
    Serial.print(AccessoryNo);
    Serial.println(" 0>");
    Update_Screen_Accessory(AccessoryNo, 0);
  }

}

void Track_Power_On() {
  Serial.println("<1>");
}

void Track_Power_Off() {
  Serial.println("<0>");
}

int Loco_Column_Centre(int k) {
  return ((k - 1) * 30) + 15;
}

void Update_Screen_Locos() {
  int k;
  int idY = 266;

  tft.setRotation(1);
  tft.setTextColor(BLACK, Z_DCC);

  for (k = 1; k <= 16; k++)   {
    int colCentre = Loco_Column_Centre(k);
    String id = String(Z1_LocoNo[k], DEC);
    int digits = id.length();

    tft.fillRect(colCentre - 14, idY, 28, 16, Z_DCC);

    if (digits <= 2) {
      tft.setTextSize(2);
      tft.setCursor(colCentre - (digits * 6), idY);
    }
    else {
      tft.setTextSize(1);
      tft.setCursor(colCentre - (digits * 3), idY + 4);
    }
    tft.print(id);
  }

  tft.setRotation(0);
}

void Update_Screen_Speeds(int k) {
  int colCentre = Loco_Column_Centre(k);
  int dirY = 242;
  int speedY = 230;
  int speed = constrain(Z1_LocoSpeed[k], 0, 126);
  int digits = 1;
  if (speed >= 100) {
    digits = 3;
  }
  else if (speed >= 10) {
    digits = 2;
  }

  tft.setRotation(1);
  tft.setTextColor(BLUE, Z_DCC);

  tft.setTextSize(1);
  tft.setCursor(colCentre - 9, speedY);
  tft.print("   ");
  tft.setCursor(colCentre - (digits * 3), speedY);
  tft.print(speed);

  tft.setTextSize(2);
  tft.setCursor(colCentre - 6, dirY);
  if (Z1_LocoDirection[k] == 1) {
    tft.print(">");
  }
  else {
    tft.print("<");
  }

  tft.setRotation(0);
}

void Update_Screen_Accessory(int a, byte state) {
  if (a < 1 || a > 99) {
    return;
  }

  int xPitch = 32;
  int yPitch = 30;
  int perRow = 15;
  int radius = 13;
  int cx = (((a - 1) % perRow) * xPitch) + (xPitch / 2);
  int cy = (((a - 1) / perRow) * yPitch) + (yPitch / 2);
  int digits = (a >= 10) ? 2 : 1;
  uint16_t fill;
  uint16_t text;

  if (state == 1) {
    fill = GREEN;
    text = BLACK;
  }
  else if (state == 0) {
    fill = RED;
    text = WHITE;
  }
  else {
    fill = Z_DCC;
    text = BLACK;
  }

  tft.setRotation(1);
  tft.fillCircle(cx, cy, radius, fill);
  tft.drawCircle(cx, cy, radius, BLACK);
  tft.setTextSize(1);
  tft.setTextColor(text);
  tft.setCursor(cx - (digits * 3) + 1, cy - 3);
  tft.print(a);
  tft.setRotation(0);
}
