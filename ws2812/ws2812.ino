///////////////////////////////////////////////////////////////
// ESP8266 NeoPixelBus example
// (c)2017, a.m.emelianov@gmail.com
//
// Requrements:
// https://github.com/Makuna/NeoPixelBus
// https://github.com/emelianov/Run
//

#include <Run.h>
#include <NeoPixelBus.h>

//Pixel count for Example 1
#define PIXELS 16
// Maximum brightness 0 - 255. Sure value below doesn't make sence.
#define BRI 24
// Board width for Example 2
#define H 4
// Board width for Example 2
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

uint8_t brightness = BRI;

void SetRandomSeed()
{
    uint32_t seed;
    seed = analogRead(A0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(A0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}


RgbColor black(0, 0, 0);

uint16_t flagGo = 0;
uint16_t flagSwitchColor = 0;
uint16_t flagShiftColor = 0;

uint32_t initialize() {
    const uint8_t PanelWidth = 4;  // 8 pixel x 8 pixel matrix of leds
    const uint8_t PanelHeight = 4;
    const uint8_t TileWidth = 1;  // laid out in 4 panels x 2 panels mosaic
    const uint8_t TileHeight = 1;
    const uint16_t PixelCount = PanelWidth * PanelHeight * TileWidth * TileHeight;

    tiles = new NeoTiles <MyPanelLayout, MyTilesLayout> (PanelWidth,PanelHeight, TileWidth, TileHeight);
    strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> (PIXELS);
    strip->Begin();
    strip->ClearTo(black);
    strip->Show();
    SetRandomSeed();
    flagGo++;
    return RUN_DELETE;
}

uint16_t posX = H-1;
uint16_t incX = 1;
uint8_t r = 0; 
uint8_t g = 0;
uint8_t b = 0;
int8_t dR;
int8_t dG;
int8_t dB;
BriColor clr(r, g, b, brightness);
uint8_t shiftFrame;

// Softly change color to new
uint32_t shiftColor() {
  if (shiftFrame > 0) {
    BriColor shift(r - dR*shiftFrame, g - dG*shiftFrame, b - dB*shiftFrame, brightness);
    for (uint16_t j = 0; j < W; j++) {
      strip->SetPixelColor(tiles->Map(posX + incX, j), shift);
      strip->SetPixelColor(tiles->Map(j, posX + incX), shift);
    }
    shiftFrame--;
    return 100;
  }
  // Set flag to run frame drawing routine
  flagGo = true;
  return RUN_NEVER;
}

// Gerenare new color
uint32_t switchColor() {
  uint8_t newR = random(brightness);
  uint8_t newG = random(brightness);
  uint8_t newB = random(brightness);
  uint8_t rnd = random(100);
  // Amplify one color
  if (rnd > 66) {
    newR *= 3;
  } else if (rnd > 33) {
    newG *= 3;
  } else {
    newB *= 3;
  }
  uint16_t s = newR + newG + newB;
  if ( s > 0) {
    newR = (uint32_t)newR * (uint32_t)brightness / s;
    newG = (uint32_t)newG * (uint32_t)brightness / s;
    newB = (uint32_t)newB * (uint32_t)brightness / s;
  }
  dR = (newR - r) >> 3;
  dG = (newG - g) >> 3;
  dB = (newB - b) >> 3;
  r = newR;
  g = newG;
  b = newB;
  clr = BriColor(r, g, b, brightness);
  shiftFrame = 7;
  flagShiftColor++;
  return RUN_NEVER;  
}

//Frame creating routine
uint32_t go() {
  posX += incX;
  strip->ClearTo(black);
  for (uint16_t j = 0; j < W; j++) {
    strip->SetPixelColor(tiles->Map(posX, j), clr);
    strip->SetPixelColor(tiles->Map(j, posX), clr);
  }
  if (posX >= H) {
    incX = -incX;
    flagSwitchColor++;
    return RUN_NEVER;
  }
  return 250;
}

// Set random color pixel at random position
uint32_t randomOn() {
  uint8_t r = random(brightness);
  uint8_t g = random(brightness);
  uint8_t b = random(brightness);
  strip->SetPixelColor(random(PIXELS), BriColor(r, g, b, brightness));
  return 100;
}
// Turn off pixel at random position
uint32_t randomOff() {
  strip->SetPixelColor(random(PIXELS), black);
  return 400;
}

// Refresh ws2812 LEDs
uint32_t show() {
  // Check if pixels redrawing engine (i mean DMA) is busy 
  if (strip->CanShow()) {
    strip->Show();
    return 50;
  }
  return 20;
}

// Changing brightness according to external light intensivity
// In general, GL5537 Photosensitive resistance is connected to Analog Input  
uint32_t setBri() {
  brightness = ((uint32_t)BRI * (1023 - analogRead(A0))) >> 10;
  return 100;
}

void setup()
{
  // Uncomment one of examples
  // Example 1
  // taskAdd(randomOn);
  // taskAdd(randomOff);
  // End of Example 1
  
  // Example 2
  taskAddWithSemaphore(go,  &flagGo);
  taskAddWithSemaphore(switchColor,  &flagSwitchColor);
  taskAddWithSemaphore(shiftColor,  &flagShiftColor);
  // End of Example 2
  taskAdd(show);
  taskAdd(setBri);
  initialize();
}

void loop()
{
  taskExec();
}

