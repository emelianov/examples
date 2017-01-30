///////////////////////////////////////////////////////////////
// ESP8266 AXDL345 + WS2812B + LCD (for debug only) example
// (c)2017, a.m.emelianov@gmail.com
//
// Requrements:
// https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
// https://github.com/Makuna/NeoPixelBus
// https://github.com/emelianov/ADXL345
// https://github.com/emelianov/Run
//

#include <Wire.h>
#include <Run.h>
#include <ADXL345.h>
#include <LiquidCrystal_I2C.h>
#include <NeoPixelBus.h>

#define MIN_ANGLE 10      // Accelerometer sensitivity
#define MIN_ACT 5         // Acceleration time
#define PP 2              // Movement events by LED
#define SPLASH 10         // Splash frame time

#define PIN_ACT       D4  //ESP-12 LED
#define PIN_ALERT     D0  //NodeMCU LED
#define BUSY          digitalWrite(PIN_ACT, LOW);
#define IDLE          digitalWrite(PIN_ACT, HIGH);
#define ALERT         digitalWrite(PIN_ALERT, LOW);
#define NOALERT       digitalWrite(PIN_ALERT, HIGH);

// Brightness 0 - 255
#define BRI 24
// Color brightness
#define RC 64
// Board height
#define H 4
// Board width
#define W 4
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>* strip;
typedef ColumnMajorAlternatingLayout MyPanelLayout;
typedef ColumnMajorLayout MyTilesLayout;
NeoTiles <MyPanelLayout, MyTilesLayout>* tiles;

// Brightness correction Class
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
BriColor black( 0, 0, 0, BRI);
BriColor blue(  0, 0,64, BRI);
BriColor green( 0,64, 0, BRI);

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
 uint16_t splash;
 uint16_t die;
 uint16_t finish;
};
handlers call = {0, 0, 0, 0, 0, 0};

#define BUTTON_INTR 0

// 
#define ADXL_INTR 	12
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

int16_t bx = 2 << PP;
int16_t by = 2 << PP; 
BriColor bc(64, 0, 0, BRI);
uint32_t xAction = 0;
uint32_t yAction = 0;
uint16_t minAngle = MIN_ANGLE;

void adxlIntr() {
 call.adxlHandler++;
 BUSY
}
void buttonIntr() {
  //call.adxlZerroHandler++;
  //adxlZerro.x = adxlLast.x;
  //adxlZerro.y = adxlLast.y;
  //adxlZerro.z = adxlLast.z;
  minAngle++;
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
  lcd.setCursor(0,1);
  lcd.print(taskCount);
  if (abs(x) > minAngle) {
    if (xAction != 0)  {
      if (millis() - xAction > MIN_ACT && call.die == 0) {
        if  (x > 0) {
          if (bx < (W << PP)) {
            bx++;
            call.display++;
          } else {
            call.die++;
            call.splash++;
          }
        }
        if (x < 0) {
          if (bx > 0) {
            bx--;
            call.display++;
          } else {
            call.die++;
            call.splash++;
          }
        }
      }
    } else {
      xAction = millis();
    }
  } else {
    xAction = 0;
  }
  if (abs(y) > minAngle) {
    if (yAction != 0)  {
      y = -y;
      if (millis() - yAction > MIN_ACT && call.die == 0) {
        if  (y > 0) {
          if (by < (H << PP)) {
            by++;
            call.display++;
          } else {
            call.die++;
            call.splash++;
          }
        }
        if (y < 0) {
          if (by > 0) {
            by--;
            call.display++;
          } else {
            call.die++;
            call.splash++;
          }
        }
      }
    } else {
      yAction = millis();
    }
  } else {
    yAction = 0;
  }
  if (bx < PP && by < PP) {
    call.finish++;
    call.die++; 
  } else {
    //call.display++;
  }
  IDLE
  return RUN_NEVER;
}

BriColor splashColor(1, 0, 0, BRI);
uint32_t start() {
  call.die = 0;
  call.display++;
  return RUN_DELETE;
}
uint32_t newGame() {
  strip->ClearTo(black);
  strip->Show();
  bx = 2 << PP;
  by = 2 << PP;
  call.display = 0;
  taskAddWithDelay(start, 2000);
  return RUN_DELETE;
}
uint32_t finish() {
  strip->ClearTo(blue);
  strip->Show();
  bx = 2 << PP;
  by = 2 << PP;
  call.display = 0;
  taskAddWithDelay(start, 2000);
  return RUN_NEVER;
}

uint32_t white() {
  splashColor.R = 255;
  splashColor.G = 255;
  splashColor.B = 255;
  strip->ClearTo(splashColor);
  strip->Show();
  splashColor.R = 1;
  splashColor.G = 0;
  splashColor.B = 0;  
  taskAddWithDelay(newGame, SPLASH);
  return RUN_DELETE;
}

uint32_t splash() {
  if (splashColor.R < 128) {
    splashColor.R <<= 1;
    strip->ClearTo(splashColor);
    strip->Show();
    return SPLASH;
  } else {
    splashColor.R = 255;
    strip->ClearTo(splashColor);
    strip->Show();
    taskAddWithDelay(white, SPLASH);
    return RUN_NEVER;
  }
}

uint32_t display() {
  //if (call.die > 0) {
    //strip->ClearTo(splashColor);
    //strip->Show();
   // return RUN_NEVER;
  //} else {
    strip->ClearTo(black);
    strip->SetPixelColor(tiles->Map(0, 0), green);
    strip->SetPixelColor(tiles->Map(bx >> PP, by >> PP), bc);
    strip->Show();
    return RUN_NEVER;
  //}
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
  strip->Begin();
  strip->ClearTo(black);
  strip->Show();
  
  Wire.begin();
  lcd.begin();
  lcd.setCursor(0,0);
  lcd.print("Hello");
  adxl.powerOn();
  byte interrupts = adxl.getInterruptSource();
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
  //byte interrupts = adxl.getInterruptSource();
  pinMode(D0, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(ADXL_INTR, INPUT);
  IDLE
  NOALERT
  attachInterrupt(ADXL_INTR, adxlIntr, RISING);
  attachInterrupt(BUTTON_INTR, buttonIntr, RISING);
  taskAddWithSemaphore(movementHandler, &(call.adxlHandler));
  taskAddWithSemaphore(display, &(call.display));
  taskAdd(newGame);
  taskAddWithSemaphore(splash, &(call.splash));
  taskAddWithSemaphore(finish, &(call.finish));
  adxl.readAccel(&adxlLast.x, &adxlLast.y, &adxlLast.z);
}

void loop()
{
  taskExec();
  yield();
}

