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

// Display I2C address
#define LCD_I2C 0x34
#define LCD_W	16
#define LCD_H	2
LiquidCrystal_I2C lcd(LCD_I2C, LCD_W, LCD_H);
LCDBitmap bitmap(&lcd, 12, 0); // 20x16 bitmap

struct handlers {
 bool adxlHandler;
};
handlers call = {false};

#define ADXL_INTR 	12
#define ADXL_HISTORY	20
ADXL345 adxl;
void adxlIntr() {
 call.adxlHandler = true;
}

struct d3d {
 int16_t x;
 int16_t y;
 int16_t z;
};
d3d adxlLast;
uint16_t adxlHistory[ADXL_HISTORY];
uint8_t adxlHistoryPos = 0;
uint16_t adxlHistoryMax = 0;

#define DELTA 1000
uint32_t movementHandler() {
  int16_t x,y,z;  
  adxl.readAccel(&x, &y, &z);
  int16_t val = abs(x - adxlLast.x) + abs(y - adxlLast.y) + abs(z - adxlLast.z);
  if (val > adxlHistoryMax) {
    adxlHistoryMax = val;
  }
  if (val > adxlHistory[adxlHistoryPos]) {
    adxlHistory[adxlHistoryPos] = val;
  }
  return RUN_NEVER;
}

uint32_t displayHandler() {
  uint16_t scale = adxlHistoryMax >> 4;
  bitmap.clear();
  for (uint8_t i = adxlHistoryPos ; ;) {
   bitmap.pixel(i, adxlHistory[i] / scale, ON);
   i++;
   if (i > ADXL_HISTORY) {
     i = 0;
   }
   if (i == adxlHistoryPos) {
     break;
   }
  }
  bitmap.update();
  adxlHistoryPos++;
  if (adxlHistoryPos > ADXL_HISTORY) {
   adxlHistoryPos = 0;
  }
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
  adxl.setActivityThreshold(2); // 75?
  adxl.setInactivityThreshold(2); // 75?
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);
  adxl.setInterruptMapping(ADXL345_INT_ACTIVITY_BIT, ADXL345_INT1_PIN);
  adxl.setInterrupt(ADXL345_INT_ACTIVITY_BIT, 1);
  pinMode(ADXL_INTR, INPUT);
  attachInterrupt(ADXL_INTR, adxlIntr, FALLING); //RAISING ?
  taskAddWithSemaphore(movementHandler, &call.adxlHandler);
  adxl.readAccel(&adxlLast.x, &adxlLast.y, &adxlLast.z);
  taskAdd(displayHandler);
}

void loop()
{
  taskExec();
}

