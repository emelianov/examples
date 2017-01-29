///////////////////////////////////////////////////////////////
// ESP8266 AXDL345 + LCD example
// (c)2017, a.m.emelianov@gmail.com
//
// Requrements:
// https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
// https://bitbucket.org/teckel12/arduino-lcd-bitmap
// https://github.com/emelianov/ADXL345
// https://github.com/emelianov/Run
//

#include <Wire.h>
#include <Run.h>
#include <ADXL345.h>
#include <LiquidCrystal_I2C.h>
#include <LCDBitmap.h>

#define PIN_ACT       D4  //Net LED
#define PIN_ALERT     D0  //16
#define BUSY          digitalWrite(PIN_ACT, LOW);
#define IDLE          digitalWrite(PIN_ACT, HIGH);
#define ALERT         digitalWrite(PIN_ALERT, LOW);
#define NOALERT       digitalWrite(PIN_ALERT, HIGH);

// Display I2C address
#define LCD_I2C 0x27
#define LCD_W	16
#define LCD_H	2
LiquidCrystal_I2C lcd(LCD_I2C, LCD_W, LCD_H);
LCDBitmap bitmap(&lcd, 12, 0); // 20x16 bitmap

struct handlers {
 uint16_t adxlHandler;
 uint16_t adxlZerroHandler;
};
handlers call = {0, 0};

#define BUTTON_INTR 0

#define ADXL_INTR 	14
#define ADXL_HISTORY	20
ADXL345 adxl;

struct d3d {
 int16_t x;
 int16_t y;
 int16_t z;
};
d3d adxlLast;
d3d adxlZerro = {0, 0, 0};
uint16_t adxlHistory[ADXL_HISTORY];
uint8_t adxlHistoryPos = 0;
uint16_t adxlHistoryMax = 0;

#define DELTA 1000

void adxlIntr() {
 call.adxlHandler++;
 BUSY
}
void buttonIntr() {
  //call.adxlZerroHandler++;
  adxlZerro.x = adxlLast.x;
  adxlZerro.y = adxlLast.y;
  adxlZerro.z = adxlLast.z;
  ALERT
}

uint32_t movementHandler() {
  int16_t x,y,z;
  adxl.readAccel(&adxlLast.x, &adxlLast.y, &adxlLast.z);
  byte interrupts = adxl.getInterruptSource();
  /*
  int16_t val = abs(x - adxlLast.x) + abs(y - adxlLast.y) + abs(z - adxlLast.z);
  if (val > adxlHistoryMax) {
    adxlHistoryMax = val;
  }
  if (val > adxlHistory[adxlHistoryPos]) {
    adxlHistory[adxlHistoryPos] = val;
  } */
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(adxlLast.x - adxlZerro.x);
  lcd.print(" ");
  lcd.print(adxlLast.y - adxlZerro.y);
  lcd.print(" ");
  lcd.print(adxlLast.z - adxlZerro.z);
  IDLE
  return RUN_NEVER;
}

uint32_t displayHandler() {
  return DELTA;
  uint16_t scale = adxlHistoryMax >> 4;
  if (scale == 0) scale = 1;
   bitmap.clear();
  for (uint8_t i = adxlHistoryPos ; ;) {
   bitmap.pixel(i, adxlHistory[i] / scale, ON);
//   lcd.setCursor(0,0);
//   lcd.print(adxlHistory[i] / scale);
   i++;
   if (i > ADXL_HISTORY) {
     i = 0;
   }
   if (i == adxlHistoryPos) {
     break;
   }
   bitmap.pixel(i, 0, ON);
  }
  bitmap.update();
  adxlHistoryPos++;
  if (adxlHistoryPos > ADXL_HISTORY) {
   adxlHistoryPos = 0;
  }
//  bitmap.pixel(adxlHistoryPos, 1, ON);
//  bitmap.update();
//  adxlHistoryPos++;
//  if (adxlHistoryPos > ADXL_HISTORY) return RUN_DELETE;
  return DELTA;
}

void setup()
{
  Wire.begin();
  lcd.begin();
  bitmap.begin();
  bitmap.home();
  adxl.powerOn();
  adxl.setRangeSetting(2);
  adxl.setActivityThreshold(3); // 75?
  adxl.setInactivityThreshold(3); // 75?
  adxl.setTimeInactivity(1);
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(0);
  adxl.setInactivityX(0);
  adxl.setInactivityY(0);
  adxl.setInactivityZ(0);
  adxl.setTapDetectionOnX(0);
  adxl.setTapDetectionOnY(0);
  adxl.setTapDetectionOnZ(0);
  adxl.setInterruptMapping(ADXL345_INT_ACTIVITY_BIT, ADXL345_INT1_PIN);
  adxl.setInterrupt(ADXL345_INT_ACTIVITY_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_FREE_FALL_BIT,  1);
  adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 1);
  byte interrupts = adxl.getInterruptSource();
  pinMode(D0, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(ADXL_INTR, INPUT);
  IDLE
  NOALERT
  attachInterrupt(ADXL_INTR, adxlIntr, RISING);
  attachInterrupt(BUTTON_INTR, buttonIntr, RISING);
  taskAddWithSemaphore(movementHandler, &(call.adxlHandler));
  adxl.readAccel(&adxlLast.x, &adxlLast.y, &adxlLast.z);
  //taskAdd(displayHandler);
}

void loop()
{
  taskExec();
  yield();
}

