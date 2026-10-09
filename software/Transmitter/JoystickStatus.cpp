#include <TFT_eSPI.h>
#include "UserInterface.h"
#include "JoystickStatus.h"
#include <VectorUtilities.h>
using namespace vectorutilities;

// ---- JoystickStatus --------------------------------------------------------------------

namespace
{

const float cRectLength = 100;           // H/V slider length
const float cRectWidth  = 15;            // H/V slider width
const float cRectR      = 8;             // H/V rounded rectangle radius
const float cSpotR      = 5;             // radius of the spot indication the current joystick position
const uint32_t cSpotCol = 0x4CBB; 
const uint32_t cBtnCol  = 0x46AB; 
const int32_t cArcR     = 28;            // Arc slider outer radius
const int32_t cArcIR    = 13;            // Arc slider inner radius

const float   cLeftXPosX = 68;           // left, x-axis, pos x
const float   cLeftXPosY = 167;          //               pos Y
const float   cLeftYPosX = 8;            //       y-axis, pos x
const float   cLeftYPosY = 7;            //               pos y
const int32_t cLeftZPosX = cLeftYPosX    //       z-axis, pos X
                         + cArcR;
const int32_t cLeftZPosY = cLeftXPosY    //               pos y
                         - cArcR;

const float cBufWidth  = cLeftXPosX+cRectLength+cRectR+1;
const float cBufHeight = cLeftXPosY+cRectWidth/2+2;

const float   cRightXPosX = cBufWidth    // right, x-axis, pos x
                          - cLeftXPosX
                          - cRectLength;     
const float   cRightXPosY = cLeftXPosY;  //                pos Y
const float   cRightYPosX = cBufWidth    //        y-axis, pos x
                          - cLeftYPosX-1;
const float   cRightYPosY = cLeftYPosY;  //                pos y
const int32_t cRightZPosX = cRightYPosX  //        z-axis, pos x
                          - cArcR;
const int32_t cRightZPosY = cRightXPosY  //                pos y
                          - cArcR;

} // namespace



// ----------------------------------------------------------------------------------------
JoystickStatus::JoystickStatus(UserInterface& ui, bool isLeft) 
: DisplayObject(ui),
  myJoystick(isLeft == true ? ui.input().joyL() : ui.input().joyR()), 
  myIsLeft(isLeft)
{}

// ----------------------------------------------------------------------------------------
void JoystickStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  if (myIsLeft == true)
  {
    if (myLastState.joyLisPressed != myUI.inputState().joyLisPressed ||
        myLastState.LX            != myUI.inputState().LX            ||
        myLastState.LY            != myUI.inputState().LY            ||
        myLastState.LZ            != myUI.inputState().LZ            ||
        myLastLabel               != myUI.input().getLabelBtnL())
    {
      markRedraw();
      myLastState = myUI.inputState();
      myLastLabel = myUI.input().getLabelBtnL();
    }
  }
  else // right side
  {
    if (myLastState.joyRisPressed != myUI.inputState().joyRisPressed ||
        myLastState.RX            != myUI.inputState().RX            ||
        myLastState.RY            != myUI.inputState().RY            ||
        myLastState.RZ            != myUI.inputState().RZ            ||
        myLastLabel               != myUI.input().getLabelBtnR())
    {
      markRedraw();
      myLastState = myUI.inputState();
      myLastLabel = myUI.input().getLabelBtnR();
    }
  }
}

// ----------------------------------------------------------------------------------------
void JoystickStatus::draw(bool forceRedraw)
{
  if (isActive() == false || (forceRedraw == false && needsRedraw() == false))
    return;

  clearRedrawFlags();

  static bool reported = false;
  if (createBuffer(int(cBufWidth), int(cBufHeight), __PRETTY_FUNCTION__, reported) == false)
    return;

  myBuf.fillSprite(TFT_TRANSPARENT);

  myBuf.setTextColor(cBorderCol, cMainAreaCol);
  myBuf.setTextPadding(myUI.tft().textWidth("0000000000", 2)); // The padding will blank any area not covered by the string.

  // Left joystick
  if (myIsLeft == true)
  {
    drawSliderH(cLeftXPosX, cLeftXPosY, myUI.inputState().LX);
    drawSliderV(cLeftYPosX, cLeftYPosY, myUI.inputState().LY);
    drawSliderA(cLeftZPosX, cLeftZPosY, myUI.inputState().LZ, myUI.inputState().joyLisPressed);
    if (myJoystick.isActive())
    {
      myBuf.setTextDatum(ML_DATUM);
      myBuf.drawString(myUI.input().getLabelBtnL(), cLeftZPosX+cArcR+7, cLeftZPosY, 2);
    }
    myBuf.pushSprite(cMainAreaPosX+8, 132, TFT_TRANSPARENT);
  }
  // Right joystick
  else 
  {
    drawSliderH(cRightXPosX, cRightXPosY, myUI.inputState().RX);
    drawSliderV(cRightYPosX, cRightYPosY, myUI.inputState().RY);
    drawSliderA(cRightZPosX, cRightZPosY, myUI.inputState().RZ, myUI.inputState().joyRisPressed);
    if (myJoystick.isActive())
    {
      myBuf.setTextDatum(MR_DATUM);
      myBuf.drawString(myUI.input().getLabelBtnR(), cRightZPosX-cArcR-7, cRightZPosY, 2);
    }
    myBuf.pushSprite(cTFTWidth-cMainAreaPosX-cBufWidth-8, 132, TFT_TRANSPARENT);
  }

  myBuf.deleteSprite();
}

// ----------------------------------------------------------------------------------------
void JoystickStatus::drawSliderH(float x, float y, int val)
{
  myBuf.fillRect(x-cRectR, y-cRectWidth/2, cRectLength+2*cRectR, cRectWidth, cMainAreaCol);
  myBuf.drawSmoothRoundRect(x-cRectR, y-cRectWidth/2, cRectR, cRectR-1, cRectLength+2*cRectR, cRectWidth, 
                            myJoystick.isActive() ? cBorderCol : cBorderColNA, TFT_BLACK);

  // Dynamic visualization of current axis position
  if (myJoystick.isActive())
  {
    if (val <= 0)
      myBuf.drawSpot(x+mapf(val, Joystick3Axis::cMIN, 0, 0, cRectLength/2), y, cSpotR, cSpotCol);
    else 
      myBuf.drawSpot(x+cRectLength/2+mapf(val, 0, Joystick3Axis::cMAX, 0, cRectLength/2), y, cSpotR, cSpotCol);
  }

  // Mid point indicator
  myBuf.drawWideLine(x+cRectLength/2, y-cRectWidth/2, x+cRectLength/2, y+cRectWidth/2, 1, 
                     myJoystick.isActive() ? cBorderCol : cBorderColNA);
}

// ----------------------------------------------------------------------------------------
void JoystickStatus::drawSliderV(float x, float y, int val)
{
  myBuf.fillRect(x-cRectR, y-cRectWidth/2, cRectWidth, cRectLength+2*cRectR-1, cMainAreaCol);
  myBuf.drawSmoothRoundRect(x-cRectR, y-cRectWidth/2, cRectR, cRectR-1, cRectWidth, cRectLength+2*cRectR-1,
                            myJoystick.isActive() ? cBorderCol : cBorderColNA, TFT_BLACK);

  // Dynamic visualization of current axis position
  if (myJoystick.isActive())
  {
    if (val <= 0)
      myBuf.drawSpot(x, y+cRectLength-mapf(val, Joystick3Axis::cMIN, 0, 0, cRectLength/2), cSpotR, cSpotCol);
    else 
      myBuf.drawSpot(x, y+cRectLength/2-mapf(val, 0, Joystick3Axis::cMAX, 0, cRectLength/2), cSpotR, cSpotCol);
  }

  // Mid point indicator
  myBuf.drawWideLine(x-cRectWidth/2, y+cRectLength/2, x+cRectWidth/2, y+cRectLength/2, 1, 
                     myJoystick.isActive() ? cBorderCol : cBorderColNA);
}

// ----------------------------------------------------------------------------------------
void JoystickStatus::drawSliderA(float x, float y, int val, bool btnPressed)
{
  myBuf.fillCircle(x, y, cArcR,  cMainAreaCol);
  myBuf.drawSmoothCircle(x, y, cArcR,  myJoystick.isActive() ? cBorderCol : cBorderColNA, TFT_BLACK);
  myBuf.drawSmoothCircle(x, y, cArcIR, myJoystick.isActive() ? cBorderCol : cBorderColNA, TFT_BLACK);

  // Dynamic visualization of current axis position
  if (myJoystick.isActive())
  {
    if (val <= 0)
    {
      float sx = -sinf((mapf(val, Joystick3Axis::cMIN, 0, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf((mapf(val, Joystick3Axis::cMIN, 0, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myBuf.drawSpot(sx, sy, cSpotR, cSpotCol);
    }
    else
    {
      float sx = -sinf(mapf(val, 0, Joystick3Axis::cMAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf(mapf(val, 0, Joystick3Axis::cMAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myBuf.drawSpot(sx, sy, cSpotR, cSpotCol);
    }
  }

  // Mid point indicator
  myBuf.drawWideLine(x, y+cArcIR, x, y+cArcR, 1, myJoystick.isActive() ? cBorderCol : cBorderColNA);

  // Button state indicator
  if (myJoystick.isActive() && btnPressed == true)
    myBuf.fillSmoothCircle(x, y, cArcIR-5, cBtnCol, TFT_BLACK);
}


