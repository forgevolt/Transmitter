#include "FPSStatus.h"
#include "FPSToggle.h"

// ---- FPSStatus -------------------------------------------------------------------------

const float cPosX = 350;
const float cPosY = 22; 

// ----------------------------------------------------------------------------------------
FPSStatus::FPSStatus(UserInterface& ui)
: DisplayObject(ui),
  myFPS(0), myFPSCounter(0), myStime(esp_timer_get_time())
{}

// ----------------------------------------------------------------------------------------
void FPSStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;
      
  // Compute FPS each second
  if (esp_timer_get_time() - myStime > 1000000) 
  {
    myFPS = myFPSCounter;
    myStime = esp_timer_get_time();
    myFPSCounter = 0;
    markRedraw();
  }
  ++myFPSCounter;
}

// ----------------------------------------------------------------------------------------
void FPSStatus::draw(bool forceRedraw)
{
  if (isActive() == false || (forceRedraw == false && needsRedraw() == false))
    return;

  clearRedrawFlags();

  if (showFPS == false)
    return;

  myUI.tft().setTextDatum(TR_DATUM);
  myUI.tft().setTextColor(cBorderCol, cStatusAreaCol);
  myUI.tft().setTextPadding(myUI.tft().textWidth("FPS", 2)); // The padding will blank any area not covered by the string.
  myUI.tft().drawNumber(myFPS, cPosX, cPosY, 2);
  myUI.tft().setTextDatum(TL_DATUM);
  myUI.tft().drawString("FPS", cPosX+4, cPosY, 2);
}

