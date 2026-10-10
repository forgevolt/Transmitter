#include "FPSStatus.h"
#include "FPSToggle.h"

// ---- FPSStatus -------------------------------------------------------------------------

namespace
{

const float cPosX = 350;
const float cPosY = 22; 

// A gap between two step() calls longer than this means step() was not called, i.e. the menu was
// open. Frames take about 33 ms, so a long one stays well below it.
constexpr int64_t cPauseUs = 500000;

} // namespace

// ----------------------------------------------------------------------------------------
FPSStatus::FPSStatus(UserInterface& ui, const FPSToggle& toggle)
: DisplayObject(ui),
  myToggle(toggle),
  myFPS(-1), myFPSCounter(0), myStime(esp_timer_get_time()), myLastStep(myStime)
{}

// ----------------------------------------------------------------------------------------
void FPSStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;
      
  const int64_t now = esp_timer_get_time();

  // step() is not called while the menu is open. After such a pause, start a new count and show no
  // value until it is complete: a count spanning the pause would be meaningless.
  if (now - myLastStep > cPauseUs)
  {
    myFPS = -1;
    myStime = now;
    myFPSCounter = 0;
    markRedraw();
  }
  myLastStep = now;

  // Compute FPS each second
  if (now - myStime > 1000000) 
  {
    myFPS = myFPSCounter;
    myStime = now;
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

  if (myToggle.isOn() == false)
    return;

  myUI.tft().setTextDatum(TR_DATUM);
  myUI.tft().setTextColor(cBorderCol, cStatusAreaCol);
  myUI.tft().setTextPadding(myUI.tft().textWidth("FPS", 2)); // The padding will blank any area not covered by the string.
  if (myFPS < 0)
    myUI.tft().drawString("--", cPosX, cPosY, 2);
  else
    myUI.tft().drawNumber(myFPS, cPosX, cPosY, 2);
  myUI.tft().setTextDatum(TL_DATUM);
  myUI.tft().drawString("FPS", cPosX+4, cPosY, 2);
}

