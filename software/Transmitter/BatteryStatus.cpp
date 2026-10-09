#include <TFT_eSPI.h>
#include "BatteryStatus.h"

// ---- BatteryStatus ---------------------------------------------------------------------

namespace
{

constexpr uint32_t c80_100 = 0x46AB;
constexpr uint32_t c60_80  = 0x9EC9;
constexpr uint32_t c40_60  = 0xFEA0; 
constexpr uint32_t c20_40  = 0xEB86;
constexpr uint32_t c00_20  = 0xC000;

constexpr float cPosX = 20;
constexpr float cPosY = 17;

constexpr unsigned long cPollIntervalMs = 30000; // the fuel gauge is read this often, not per frame

} // namespace

// ----------------------------------------------------------------------------------------
BatteryStatus::BatteryStatus(UserInterface& ui, Adafruit_MAX17048& batteryMonitor)
: DisplayObject(ui),
  myFuelGauge(batteryMonitor),
  myLastUpdate(millis()),
  myCellPercent(-1)
{}

// ----------------------------------------------------------------------------------------
void BatteryStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;
      
  // Update every 30sec
  if (millis() - myLastUpdate > cPollIntervalMs || myCellPercent < 0)
  { 
    myLastUpdate = millis();
    myCellPercent = myFuelGauge.cellPercent();
    markRedraw();
  }  
}

// ----------------------------------------------------------------------------------------
void BatteryStatus::draw(bool forceRedraw)
{
  if (isActive() == false || (forceRedraw == false && needsRedraw() == false))
    return;

  clearRedrawFlags();

  // Clear before redrawing
  myUI.tft().fillRect(cPosX-1, cStatusAreaPosY, 53, cStatusAreaHeight, cStatusAreaCol);

  uint32_t col;
  int numSeg;
 
  if (myCellPercent > 80)
  {
    col = c80_100;
    numSeg = 5;
  }
  else if (myCellPercent > 60 && myCellPercent <= 80)
  {
    col = c60_80;
    numSeg = 4; 
  }
  else if (myCellPercent > 40 && myCellPercent <= 60)
  {
    col = c40_60;
    numSeg = 3;
  }
  else if (myCellPercent > 20 && myCellPercent <= 40)
  {
    col = c20_40;
    numSeg = 2;
  }
  else
  {
    col = c00_20;
    numSeg = 1;
  }

  // Shape of the battery
  myUI.tft().fillSmoothRoundRect(cPosX+46, cPosY+7, 5, 12, 2, cBorderCol, TFT_BLACK);
  myUI.tft().drawSmoothRoundRect(cPosX, cPosY, 4, 3, 46, 25, cBorderCol, TFT_BLACK);

  // Visualize charge percentage
  myUI.tft().fillRect(cPosX+4, cPosY+4, numSeg*8-1, 18, col);
  for (int i=1; i<numSeg; i++)
  {
    myUI.tft().drawFastVLine(cPosX+4+i*8-1, cPosY+4, 18, TFT_BLACK);
  }
}

