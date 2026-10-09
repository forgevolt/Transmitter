#include <TFT_eSPI.h>

#include <Streaming.h>
#include "JoystickCalibration.h"
#include "Utilities.h"


// ---- JoystickCalibration ---------------------------------------------------------------

// Center calibration lasts 6 seconds (3 seconds wait, 3 seconds measurements)
const unsigned long cCenterWaitDuration  = 3000; // wait time until the center measurements begin
const unsigned long cCenterTotalDuration = 6000; 

// Total of 10 seconds to move all axis (max 6) to min/max positions
const unsigned long cMinMaxDuration = 10000;

const float cRectLength = 100;           // H/V slider length
const float cRectWidth  = 15;            // H/V slider width
const float cRectR      = 8;             // H/V rounded rectangle radius
const float cSpotR      = 5;             // radius of the spot indication the current joystick position
const int32_t cArcR     = 28;            // Arc slider outer radius
const int32_t cArcIR    = 13;            // Arc slider inner radius

const float   cLeftXPosX = 87;           // left, x-axis, pos x
const float   cLeftXPosY = 300;          //               pos Y
const float   cLeftYPosX = 27;           //       y-axis, pos x
const float   cLeftYPosY = 140;          //               pos y
const int32_t cLeftZPosX = cLeftYPosX    //       z-axis, pos X
                         + cArcR;
const int32_t cLeftZPosY = cLeftXPosY    //               pos y
                         - cArcR;

const float   cRightXPosX = cTFTWidth    // right, x-axis, pos x
                          - cLeftXPosX
                          - cRectLength;     
const float   cRightXPosY = cLeftXPosY;  //                pos Y
const float   cRightYPosX = cTFTWidth    //        y-axis, pos x
                          - cLeftYPosX;
const float   cRightYPosY = cLeftYPosY;  //                pos y
const int32_t cRightZPosX = cRightYPosX  //        z-axis, pos x
                          - cArcR;
const int32_t cRightZPosY = cRightXPosY  //                pos y
                          - cArcR;

// ----------------------------------------------------------------------------------------
JoystickCalibration::JoystickCalibration(UserInterface& ui, Menu& menu)
: MenuItem(ui, menu, "Joystick Calibration"),
  myJoyLeft(ui.input().joyL()),
  myJoyRight(ui.input().joyR())
{}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  const unsigned long elapsed = millis() - myCalibrationStartTime;

  switch (myPhase)
  {
    case EPhase::eIdle:
      break;

    case EPhase::eCenterWait:
      if (elapsed > cCenterWaitDuration)
      {
        myPhase = EPhase::eCenterMeasure;
        markPartialRedraw();
      }
      break;

    case EPhase::eCenterMeasure:
      if (elapsed <= cCenterTotalDuration)
        accumulateCenter();
      else
        startMinMax();
      break;

    case EPhase::eMinMax:
      if (myStopRequested == true || elapsed > cCenterTotalDuration + cMinMaxDuration)
        finish();
      else
        accumulateMinMax();
      break;
  }
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::accumulateCenter()
{
  myCount++;
  if (myJoyLeft.isActive())
  {
    mySumLX += myJoyLeft.getRawX();
    mySumLY += myJoyLeft.getRawY();
    mySumLZ += myJoyLeft.getRawZ();
  }

  if (myJoyRight.isActive())
  {
    mySumRX += myJoyRight.getRawX();
    mySumRY += myJoyRight.getRawY();
    mySumRZ += myJoyRight.getRawZ();
  }
}

// ----------------------------------------------------------------------------------------
// Averages the center samples, seeds the min/max travel with them and hands over to eMinMax.
void JoystickCalibration::startMinMax()
{
  // Unlikely, but better safe than sorry (division by zero)
  if (myCount == 0)
  {
    myPhase = EPhase::eIdle;
    setState(EState::eNotActive);
    Serial << __PRETTY_FUNCTION__  << " myCount == 0 " << endl;
    return;
  }

  myCenterLX = mySumLX / myCount;
  myCenterLY = mySumLY / myCount;
  myCenterLZ = mySumLZ / myCount;

  myMinLX = myMaxLX = myCenterLX;
  myMinLY = myMaxLY = myCenterLY;
  myMinLZ = myMaxLZ = myCenterLZ;

  myCenterRX = mySumRX / myCount;
  myCenterRY = mySumRY / myCount;
  myCenterRZ = mySumRZ / myCount;

  myMinRX = myMaxRX = myCenterRX;
  myMinRY = myMaxRY = myCenterRY;
  myMinRZ = myMaxRZ = myCenterRZ;

  myPhase         = EPhase::eMinMax;
  myStopRequested = false;   // a press during the center phase does not skip this one
  myUI.signal();
  markRedraw();
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::accumulateMinMax()
{
  if (myJoyLeft.isActive())
  {
    const int x = myJoyLeft.getRawX();
    const int y = myJoyLeft.getRawY();
    const int z = myJoyLeft.getRawZ();

    myMinLX = min(myMinLX, x);  myMaxLX = max(myMaxLX, x);
    myMinLY = min(myMinLY, y);  myMaxLY = max(myMaxLY, y);
    myMinLZ = min(myMinLZ, z);  myMaxLZ = max(myMaxLZ, z);
  }

  if (myJoyRight.isActive())
  {
    const int x = myJoyRight.getRawX();
    const int y = myJoyRight.getRawY();
    const int z = myJoyRight.getRawZ();

    myMinRX = min(myMinRX, x);  myMaxRX = max(myMaxRX, x);
    myMinRY = min(myMinRY, y);  myMaxRY = max(myMaxRY, y);
    myMinRZ = min(myMinRZ, z);  myMaxRZ = max(myMaxRZ, z);
  }
}

// ----------------------------------------------------------------------------------------
// Both ways out of eMinMax - the button and the timeout - store what has been measured. The
// on-screen prompt is "Press button to exit", and an early exit still carries a full set of center
// values plus whatever travel the user swept.
void JoystickCalibration::finish()
{
  myPhase = EPhase::eIdle;
  setState(EState::eNotActive);

  if (myJoyLeft.isActive())
    myJoyLeft.setCalibrationData(myMinLX, myMaxLX, myCenterLX, myMinLY, myMaxLY, myCenterLY, myMinLZ, myMaxLZ, myCenterLZ);
  if (myJoyRight.isActive())
    myJoyRight.setCalibrationData(myMinRX, myMaxRX, myCenterRX, myMinRY, myMaxRY, myCenterRY, myMinRZ, myMaxRZ, myCenterRZ);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::draw(bool forceRedraw)
{
  if (isActive() == false)
  {
    MenuItem::draw(forceRedraw);
    return;
  }

  switch (myPhase)
  {
    case EPhase::eIdle:
      break;

    case EPhase::eCenterWait:
    case EPhase::eCenterMeasure:
      drawCenterScreen(forceRedraw);
      break;

    case EPhase::eMinMax:
      drawMinMaxScreen(forceRedraw);
      break;
  }

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawCenterScreen(bool forceRedraw)
{
  if (forceRedraw == true || needsRedraw() == true)
  {
    myUI.tft().fillScreen(TFT_BLACK);
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString("Center Position Calibration", myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
  }

  if (myPhase == EPhase::eCenterWait)
  {
    // Remaining time is computed in unsigned arithmetic, the same idiom step() uses, so it stays
    // correct across the millis() rollover. Only the finished value becomes a float.
    const unsigned long elapsed   = millis() - myCalibrationStartTime;
    const unsigned long remaining = (elapsed < cCenterWaitDuration) ? cCenterWaitDuration - elapsed : 0;

    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE, TFT_BLACK);
    myUI.tft().drawString("Calibration starts in ...", myUI.width()/2, 80, 4);
    myUI.tft().setTextPadding(myUI.tft().textWidth("00", 4)); // The padding will blank any area not covered by the string.
    myUI.tft().drawFloat(remaining/1000.0f, 0, myUI.width()/2, 110, 4);
    myUI.tft().setTextPadding(0);
  }
  else 
  {
    if (needsPartialRedraw() == true)
      myUI.tft().fillRect(0, 80, myUI.width(), 120, TFT_BLACK);

    myUI.tft().drawString("Don't move the Joysticks!", myUI.width()/2, 80, 4);
  }

  // ----- Progress bar
  myUI.tft().drawSmoothRoundRect(100-cRectR, 170-cRectWidth/2, cRectR, cRectR-1, myUI.width()-200+2*cRectR, cRectWidth, cBorderCol, TFT_BLACK);
  myUI.tft().drawWideLine(100, 170, 100+float(myUI.width()-200)/cCenterTotalDuration*(millis()-myCalibrationStartTime), 170, cRectWidth-6, TFT_BLUE, TFT_BLUE);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawMinMaxScreen(bool forceRedraw)
{
  if (forceRedraw == true || needsRedraw() == true)
  {
    myUI.tft().fillScreen(TFT_BLACK);

    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString("Min/Max Calibration", myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
    myUI.tft().drawString("Move joysticks on all axes to", myUI.width()/2, 80, 4);
    myUI.tft().drawString("min/max positions.", myUI.width()/2, 110, 4);
    myUI.tft().drawString("Press button to exit.", myUI.width()/2, 200, 4);
  }

  // ----- Progress bar
  myUI.tft().drawSmoothRoundRect(100-cRectR, 170-cRectWidth/2, cRectR, cRectR-1, myUI.width()-200+2*cRectR, cRectWidth, cBorderCol, TFT_BLACK);
  myUI.tft().drawWideLine(100, 170, 
                                  100+min(float(myUI.width()-200)/cMinMaxDuration*(millis()-myCalibrationStartTime-cCenterTotalDuration), 
                                          float(myUI.width()-200)), 
                                  170, cRectWidth-6, TFT_BLUE, TFT_BLUE);

  // ----- Left joystick
  drawSliderH(cLeftXPosX, cLeftXPosY, myMinLX, myMaxLX, myCenterLX, myJoyLeft.getRawX(), myJoyLeft.isActive());
  drawSliderV(cLeftYPosX, cLeftYPosY, myMinLY, myMaxLY, myCenterLY, myJoyLeft.getRawY(), myJoyLeft.isActive());
  drawSliderA(cLeftZPosX, cLeftZPosY, myMinLZ, myMaxLZ, myCenterLZ, myJoyLeft.getRawZ(), myJoyLeft.isActive());

  // ----- Right joystick
  drawSliderH(cRightXPosX, cRightXPosY, myMinRX, myMaxRX, myCenterRX, myJoyRight.getRawX(), myJoyRight.isActive());
  drawSliderV(cRightYPosX, cRightYPosY, myMinRY, myMaxRY, myCenterRY, myJoyRight.getRawY(), myJoyRight.isActive());
  drawSliderA(cRightZPosX, cRightZPosY, myMinRZ, myMaxRZ, myCenterRZ, myJoyRight.getRawZ(), myJoyRight.isActive());
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::start()
{
  myCount = 0;
  mySumLX = mySumLY = mySumLZ = 0;
  mySumRX = mySumRY = mySumRZ = 0;

  myPhase                = EPhase::eCenterWait;
  myStopRequested        = false;
  myCalibrationStartTime = millis();

  markRedraw();
  setState(EState::eActive);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::onClick()
{
  myStopRequested = true;
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawSliderH(float x, float y, int min, int max, int center, uint16_t raw, bool active)
{
  myUI.tft().drawSmoothRoundRect(x-cRectR, y-cRectWidth/2, cRectR, cRectR-1, cRectLength+2*cRectR, cRectWidth, 
                                    active ? cBorderCol : cBorderColNA, TFT_BLACK);

  if (active)
  {
    // Distance covered so far
    myUI.tft().drawWideLine(x+cRectLength/2+mapf(max, center, Joystick3Axis::cADC_MAX, 0, cRectLength/2), y, 
                               x+mapf(min, Joystick3Axis::cADC_MIN, center, 0, cRectLength/2), y, cRectWidth-6, TFT_BLUE, TFT_BLUE);

    // Dynamic visualization of current axis position
    if (raw <= center)
      myUI.tft().drawSpot(x+mapf(raw, Joystick3Axis::cADC_MIN, center, 0, cRectLength/2), y, cSpotR, TFT_NAVY, TFT_NAVY);
    else 
      myUI.tft().drawSpot(x+cRectLength/2+mapf(raw, center, Joystick3Axis::cADC_MAX, 0, cRectLength/2), y, cSpotR, TFT_NAVY, TFT_NAVY);
  }

  // Mid point indicator
  myUI.tft().drawWideLine(x+cRectLength/2, y-cRectWidth/2, x+cRectLength/2, y+cRectWidth/2, 1, 
                             active ? cBorderCol : cBorderColNA);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawSliderV(float x, float y, int min, int max, int center, uint16_t raw, bool active)
{
  myUI.tft().drawSmoothRoundRect(x-cRectR, y-cRectWidth/2, cRectR, cRectR-1, cRectWidth, cRectLength+2*cRectR-1,
                                    active ? cBorderCol : cBorderColNA, TFT_BLACK);

  if (active)
  {
    // Distance covered so far
    myUI.tft().drawWideLine(x, y+cRectLength/2-mapf(max, center, Joystick3Axis::cADC_MAX, 0, cRectLength/2), 
                               x, y+cRectLength-mapf(min, Joystick3Axis::cADC_MIN, center, 0, cRectLength/2), cRectWidth-6, TFT_BLUE, TFT_BLUE);

    // Dynamic visualization of current axis position
    if (raw <= center)
      myUI.tft().drawSpot(x, y+cRectLength-mapf(raw, Joystick3Axis::cADC_MIN, center, 0, cRectLength/2), cSpotR, TFT_NAVY, TFT_NAVY);
    else 
      myUI.tft().drawSpot(x, y+cRectLength/2-mapf(raw, center, Joystick3Axis::cADC_MAX, 0, cRectLength/2), cSpotR, TFT_NAVY, TFT_NAVY);
  }

  // Mid point indicator
  myUI.tft().drawWideLine(x-cRectWidth/2, y+cRectLength/2, x+cRectWidth/2, y+cRectLength/2, 1, 
                             active ? cBorderCol : cBorderColNA);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawSliderA(float x, float y, int min, int max, int center, uint16_t raw, bool active)
{
  myUI.tft().drawSmoothCircle(x, y, cArcR,  active ? cBorderCol : cBorderColNA, TFT_BLACK);
  myUI.tft().drawSmoothCircle(x, y, cArcIR, active ? cBorderCol : cBorderColNA, TFT_BLACK);

  if (active)
  {
    // Distance covered so far
    myUI.tft().drawSmoothArc(x, y, cArcR-4, cArcIR+3, 
                                mapf(min, Joystick3Axis::cADC_MIN, center, 185, 360), 
                                mapf(max, center, Joystick3Axis::cADC_MAX, 0, 175), TFT_BLUE, TFT_BLUE, true);

    // Dynamic visualization of current axis position
    if (raw <= center)
    {
      float sx = -sinf((mapf(raw, Joystick3Axis::cADC_MIN, center, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf((mapf(raw, Joystick3Axis::cADC_MIN, center, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myUI.tft().drawSpot(sx, sy, cSpotR, TFT_NAVY, TFT_NAVY);
    }
    else
    {
      float sx = -sinf(mapf(raw, center, Joystick3Axis::cADC_MAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf(mapf(raw, center, Joystick3Axis::cADC_MAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myUI.tft().drawSpot(sx, sy, cSpotR, TFT_NAVY, TFT_NAVY);
    }
  }

  // Mid point indicator
  myUI.tft().drawWideLine(x, y+cArcIR, x, y+cArcR, 1, active ? cBorderCol : cBorderColNA);  
}


