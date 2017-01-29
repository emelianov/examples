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
#include <NeoPixelBus.h>

#define MIN_ANGLE 30
#define MIN_ACT 100

#define PIN_ACT       D4  //Net LED
#define PIN_ALERT     D0  //16
#define BUSY          digitalWrite(PIN_ACT, LOW);
#define IDLE          digitalWrite(PIN_ACT, HIGH);
#define ALERT         digitalWrite(PIN_ALERT, LOW);
#define NOALERT       digitalWrite(PIN_ALERT, HIGH);

// Maximum brightness 0 - 255. Sure value below doesn't make sence.
#define BRI 24
// Board height
#define H 4
// Board width
#define W 4
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>* strip;
typedef ColumnMajorAlternatingLayout MyPanelLayout;
typedef ColumnMajorLayout MyTilesLayout;
NeoTiles <MyPanelLayout, MyTilesLayout>* tiles;

class BriColor : public RgbColor {
  public:
  BriColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bri) {
    uint16_t s = r + g + b;
    if ( s > 0) {
      R = (uint32_t)r * (uint32_t)bri / s;
      G = (uint32_t)g * (uint32_t)bri / s;
      B = (uint32_t)b * (uint32_t)bri / s;
    }

  }
};
RgbColor black(0, 0, 0);

uint8_t brightness = BRI;

// Display I2C address
#define LCD_I2C 0x27
#define LCD_W	16
#define LCD_H	2
LiquidCrystal_I2C lcd(LCD_I2C, LCD_W, LCD_H);


struct handlers {
 uint16_t adxlHandler;
 uint16_t adxlZerroHandler;
 uint16_t display;
};
handlers call = {0, 0, 0};

#define BUTTON_INTR 0

#define ADXL_INTR 	0
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

int16_t bx = 2;
int16_t by = 2; 
BriColor bc(64, 0, 0, BRI);
uint32_t xAction = 0;
uint32_t yAction = 0;

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
  x = adxlLast.x - adxlZerro.x;
  y = adxlLast.y - adxlZerro.y;
  z = adxlLast.z - adxlZerro.z;
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(x);
  lcd.print(" ");
  lcd.print(y);
  lcd.print(" ");
  lcd.print(z);
  if (abs(x) > MIN_ANGLE) {
    if (xAction != 0)  {
      if (millis() - xAction > MIN_ACT) {
        bx += signbit(x);
        if (bx < 0) bx = 0;
        if (bx >= W) bx = W - 1;
      }
    } else {
      xAction = millis();
    }
  } else {
    xAction = 0;
  }
  if (abs(y) > MIN_ANGLE) {
    if (yAction != 0)  {
      if (millis() - yAction > MIN_ACT) {
        bx += signbit(y);
        if (by < 0) by = 0;
        if (by >= W) by = W - 1;
      }
    } else {
      yAction = millis();
    }
  } else {
    yAction = 0;
  }  IDLE
  call.display++;
  return RUN_NEVER;
}

uint32_t display() {
  strip->ClearTo(black);
  strip->SetPixelColor(tiles->Map(bx, by), bc);
  strip->Show();
  return 100;
}

void setup()
{
    const uint8_t PanelWidth = W;
    const uint8_t PanelHeight = H;
    const uint8_t TileWidth = 1;
    const uint8_t TileHeight = 1;
    const uint16_t PixelCount = PanelWidth * PanelHeight * TileWidth * TileHeight;

    tiles = new NeoTiles <MyPanelLayout, MyTilesLayout> (PanelWidth,PanelHeight, TileWidth, TileHeight);
    strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> (PixelCount);
  //  strip->Begin();
  //  strip->ClearTo(black);
  //  strip->Show();
  
  Wire.begin();
  lcd.begin();
  adxl.powerOn();
  adxl.setRangeSetting(2);
  adxl.setActivityThreshold(2); // 75?
  adxl.setInactivityThreshold(2); // 75?
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
  //attachInterrupt(BUTTON_INTR, buttonIntr, RISING);
  taskAddWithSemaphore(movementHandler, &(call.adxlHandler));
 // taskAddWithDelay(display, 100, &(call.display));
  adxl.readAccel(&adxlLast.x, &adxlLast.y, &adxlLast.z);
}

void loop()
{
  taskExec();
  yield();
}

