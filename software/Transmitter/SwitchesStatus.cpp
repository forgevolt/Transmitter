#include <TFT_eSPI.h>
#include "UserInterface.h"
#include <Streaming.h>
#include "SwitchesStatus.h"
#include "Switches.h"

// ---- SwitchesStatus --------------------------------------------------------------------

namespace
{

// Constants related to the graphical objects
const float cRectWidth   = 15.0f;    // total length is cRectWidth + 2*cRectR
const float cRectHeight  = 15.0f;
const float cRectR       = 8.0f;
const float cSpotR       = 6.0f;
const uint32_t cHighCol  = 0x46AB;

const int32_t cPosY     = cMainAreaPosY + 20;
const int32_t cInc      = 40;

} // namespace

// ----------------------------------------------------------------------------------------
SwitchesStatus::SwitchesStatus(UserInterface& ui)
: DisplayObject(ui)
{}

// ----------------------------------------------------------------------------------------
void SwitchesStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  if (needsRedraw() == false)
  {
    if (myLastState.s1 != myUI.inputState().s1 ||
        myLastState.s2 != myUI.inputState().s2 ||
        myLastState.s3 != myUI.inputState().s3 ||
        myLastState.s4 != myUI.inputState().s4 ||
        myLastLabelS1  != myUI.input().switches().getLabelS1() ||
        myLastLabelS2  != myUI.input().switches().getLabelS2() ||
        myLastLabelS3  != myUI.input().switches().getLabelS3() ||
        myLastLabelS4  != myUI.input().switches().getLabelS4())
    {
      markRedraw();
      myLastState   = myUI.inputState();
      myLastLabelS1 = myUI.input().switches().getLabelS1();
      myLastLabelS2 = myUI.input().switches().getLabelS2();
      myLastLabelS3 = myUI.input().switches().getLabelS3();
      myLastLabelS4 = myUI.input().switches().getLabelS4();
    }
  }
}

// ----------------------------------------------------------------------------------------
void SwitchesStatus::draw(bool forceRedraw)
{
  if (isActive() == false || (forceRedraw == false && needsRedraw() == false))
    return;

  clearRedrawFlags();

  int16_t mid = myUI.width()/2;

  myUI.tft().setTextDatum(TC_DATUM);
  myUI.tft().setTextColor(cBorderCol, cMainAreaCol);
  myUI.tft().setTextPadding(myUI.tft().textWidth("0000000000", 2)); // The padding will blank any area not covered by the string.

  // Drawn from the same snapshot step() compares against, so the redraw trigger and the pixels
  // always describe one sample.
  drawState(mid-3*cInc, cPosY, myUI.inputState().s1, myUI.input().switches().getLabelS1());
  drawState(mid-cInc,   cPosY, myUI.inputState().s2, myUI.input().switches().getLabelS2());
  drawState(mid+cInc,   cPosY, myUI.inputState().s3, myUI.input().switches().getLabelS3());
  drawState(mid+3*cInc, cPosY, myUI.inputState().s4, myUI.input().switches().getLabelS4());
}

// ----------------------------------------------------------------------------------------
void SwitchesStatus::drawState(float x, float y, bool on, const String& label)
{
  myUI.tft().drawSmoothRoundRect(x-cRectWidth/2-cRectR, y-cRectHeight/2,
                                    cRectR, cRectR-1,
                                    cRectWidth+2*cRectR, cRectHeight,
                                    cBorderCol, TFT_BLACK);

  if (on == true)
  {
    myUI.tft().drawSpot(x-cRectWidth/2.0f, y, cSpotR, cMainAreaCol, cMainAreaCol);
    myUI.tft().drawSpot(x+cRectWidth/2.0f-1.0f, y, cSpotR, cHighCol, cMainAreaCol);
  }
  else
  {
    myUI.tft().drawSpot(x-cRectWidth/2.0f, y, cSpotR, cBorderCol, cMainAreaCol);
    myUI.tft().drawSpot(x+cRectWidth/2.0f-1.0f, y, cSpotR, cMainAreaCol, cMainAreaCol);
  }

  myUI.tft().drawString(label, x, y+10, 2);
}

