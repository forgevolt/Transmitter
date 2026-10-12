#include <TFT_eSPI.h>

#include <Streaming.h>
#include "JoystickCalibration.h"
#include <VectorUtilities.h>
using namespace vectorutilities;
using namespace joystickutilities;


// ---- JoystickCalibration ---------------------------------------------------------------

namespace
{

// Raw range of an uncalibrated axis; the sliders are drawn against it
const int cADC_MIN = 0;
const int cADC_MAX = AnalogConfig().adcMax;

// The center is measured after releases from six directions. Measured on both transmitters: a
// released stick is still after 0.3 - 0.5 s, and at rest the noise is up to 8 counts in 100 ms.
const unsigned long cStillDuration   = 200; // every axis within cStillCounts for this long (ms) ...
const int           cStillCounts     = 15;  // ... counts as holding still
const unsigned long cMeasureDuration = 300; // then the rest position is averaged for this long (ms)

// Per step: the axis, the direction of the push (0: the first twist either way, the second the
// other way) and the prompt
struct ReleaseStep
{
  size_t axis;
  int sign;
  const char* prompt;
};

const ReleaseStep cReleaseSteps[] =
{
  { 1, +1, "up" },          { 1, -1, "down" },
  { 0, -1, "to the left" }, { 0, +1, "to the right" },
  { 2,  0, "one way" },     { 2,  0, "the other way" }
};

const int cNumReleaseSteps = sizeof(cReleaseSteps) / sizeof(cReleaseSteps[0]);

// Total of 10 seconds to move all axes (up to six) to min/max positions
const unsigned long cMinMaxDuration = 10000;

// How long the list of axes that were not accepted stays on screen (unless the button is pressed)
const unsigned long cResultDuration = 5000;

const float cRectLength = 100;           // H/V slider length
const float cRectWidth  = 15;            // H/V slider width
const float cRectR      = 8;             // H/V rounded rectangle radius
const float cSpotR      = 5;             // radius of the spot indicating the current joystick position
const int32_t cArcR     = 28;            // Arc slider outer radius
const int32_t cArcIR    = 13;            // Arc slider inner radius

const float   cLeftXPosX = 87;           // left, x-axis, pos x
const float   cLeftXPosY = 300;          //               pos y
const float   cLeftYPosX = 27;           //       y-axis, pos x
const float   cLeftYPosY = 140;          //               pos y
const int32_t cLeftZPosX = cLeftYPosX    //       z-axis, pos x
                         + cArcR;
const int32_t cLeftZPosY = cLeftXPosY    //               pos y
                         - cArcR;

const float   cRightXPosX = cTFTWidth    // right, x-axis, pos x
                          - cLeftXPosX
                          - cRectLength;
const float   cRightXPosY = cLeftXPosY;  //                pos y
const float   cRightYPosX = cTFTWidth    //        y-axis, pos x
                          - cLeftYPosX;
const float   cRightYPosY = cLeftYPosY;  //                pos y
const int32_t cRightZPosX = cRightYPosX  //        z-axis, pos x
                          - cArcR;
const int32_t cRightZPosY = cRightXPosY  //                pos y
                          - cArcR;

// The axes of one joystick that were not accepted, e.g. "X, Z". Empty if all were accepted.
String rejectedAxes(const CalibrationResult<3>& result)
{
  String axes;
  if (result.accepted[0] == false) axes += "X";
  if (result.accepted[1] == false) axes += (axes.isEmpty() ? "Y" : ", Y");
  if (result.accepted[2] == false) axes += (axes.isEmpty() ? "Z" : ", Z");
  return axes;
}

// Applies a measured calibration and stores what the stick then uses: a rejected axis stores the
// full range it fell back to. An inactive stick reports all axes as accepted and stores nothing.
CalibrationResult<3> applyCalibration(InputController::Joystick& stick, const RangeTracker<3>& range,
                                      const char* nameSpace)
{
  const CalibrationResult<3> result = stick.setCalibration(range.calibration());
  if (stick.isActive())
    saveCalibration(stick, nameSpace);

  if (result.allAccepted() == false)
    Serial << __PRETTY_FUNCTION__ << " -> " << nameSpace << ", axis " << rejectedAxes(result)
           << ": no usable travel -> using full range" << endl;
  return result;
}

} // namespace

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

    case EPhase::eCenter:
      stepRelease(myJoyLeft,  myReleaseLeft,  InputController::cNamespaceJoyL);
      stepRelease(myJoyRight, myReleaseRight, InputController::cNamespaceJoyR);

      // Next step once every active joystick is done
      if (myReleaseLeft.state == ERelease::eDone && myReleaseRight.state == ERelease::eDone)
      {
        myUI.signal();
        if (++myReleaseStep < cNumReleaseSteps)
          startReleaseStep();
        else
          startMinMax();
      }
      break;

    case EPhase::eMinMax:
      if (myStopRequested == true || elapsed > cMinMaxDuration)
        finish();
      else
        accumulateMinMax();
      break;

    case EPhase::eResult:
      if (myStopRequested == true || millis() - myResultStartTime > cResultDuration)
      {
        myPhase = EPhase::eIdle;
        setState(EState::eNotActive);
      }
      break;
  }
}

// ----------------------------------------------------------------------------------------
// An inactive joystick has nothing to do.
void JoystickCalibration::startReleaseStep()
{
  myReleaseLeft.state  = myJoyLeft.isActive()  ? ERelease::ePush : ERelease::eDone;
  myReleaseRight.state = myJoyRight.isActive() ? ERelease::ePush : ERelease::eDone;
  markRedraw();
}

// ----------------------------------------------------------------------------------------
// Advances one joystick through the current step. A push in the step's direction starts the
// release over, also while it is settling or being measured.
void JoystickCalibration::stepRelease(InputController::Joystick& stick, CenterRelease& release,
                                      const char* nameSpace)
{
  if (release.state == ERelease::eDone)
    return;

  const ReleaseStep& step = cReleaseSteps[myReleaseStep];
  const DirectionConfig directions;
  const ERelease before = release.state;

  const int value = stick.get(step.axis);
  int deflection = value * step.sign;
  if (step.sign == 0)
    deflection = (release.twistSign == 0) ? abs(value) : -value * release.twistSign;

  if (deflection > directions.onThreshold)
  {
    release.state    = ERelease::eLetGo;
    release.pushSign = (value > 0) ? 1 : -1;
  }

  switch (release.state)
  {
    case ERelease::ePush:
    case ERelease::eDone:
      break;

    case ERelease::eLetGo:
      if (abs(stick.getX()) < directions.offThreshold &&
          abs(stick.getY()) < directions.offThreshold &&
          abs(stick.getZ()) < directions.offThreshold)
      {
        release.state = ERelease::eSettle;
        release.since = millis();
        for (size_t i = 0; i < 3; i++)   // the still window starts here
          release.lo[i] = release.hi[i] = stick.readRaw(i);
      }
      break;

    case ERelease::eSettle:
    case ERelease::eMeasure:
      if (isStill(stick, release) == false)
      {
        release.state = ERelease::eSettle;
        release.since = millis();
        break;
      }

      if (release.state == ERelease::eSettle && millis() - release.since >= cStillDuration)
      {
        release.state = ERelease::eMeasure;
        release.since = millis();
        release.sampler.reset();
      }

      if (release.state == ERelease::eMeasure)
      {
        release.sampler.add(stick);
        if (millis() - release.since >= cMeasureDuration)
        {
          const std::array<int, 3> rest = release.sampler.center();
          for (size_t i = 0; i < 3; i++)
            release.sum[i] += rest[i];
          release.count++;
          release.state = ERelease::eDone;
          if (step.sign == 0 && release.twistSign == 0)
            release.twistSign = release.pushSign;

          Serial << nameSpace << ": release " << (myReleaseStep + 1) << " of " << cNumReleaseSteps
                 << " at rest " << rest[0] << " " << rest[1] << " " << rest[2] << endl;
        }
      }
      break;
  }

  if (release.state != before)
    markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
// Widens the still window by one raw sample. When an axis moves further than cStillCounts, the
// window starts again from this sample and the result is false.
bool JoystickCalibration::isStill(const InputController::Joystick& stick, CenterRelease& release)
{
  std::array<int, 3> raw;
  bool still = true;

  for (size_t i = 0; i < 3; i++)
  {
    raw[i] = stick.readRaw(i);
    release.lo[i] = min(release.lo[i], raw[i]);
    release.hi[i] = max(release.hi[i], raw[i]);
    if (release.hi[i] - release.lo[i] > cStillCounts)
      still = false;
  }

  if (still == false)
    release.lo = release.hi = raw;

  return still;
}

// ----------------------------------------------------------------------------------------
// Rounded average of the rest positions; zero for an inactive joystick, which has none.
std::array<int, 3> JoystickCalibration::releaseCenter(const CenterRelease& release)
{
  std::array<int, 3> center {};
  if (release.count > 0)
    for (size_t i = 0; i < 3; i++)
      center[i] = int((release.sum[i] + release.count/2) / release.count);
  return center;
}

// ----------------------------------------------------------------------------------------
// Seeds the min/max travel with the center from the releases and hands over to eMinMax.
void JoystickCalibration::startMinMax()
{
  myRangeLeft  = RangeTracker<3>(releaseCenter(myReleaseLeft));
  myRangeRight = RangeTracker<3>(releaseCenter(myReleaseRight));

  myPhase                = EPhase::eMinMax;
  myStopRequested        = false;   // a press during the center phase does not skip this one
  myCalibrationStartTime = millis();
  markRedraw();
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::accumulateMinMax()
{
  // An inactive stick is ignored
  myRangeLeft.add(myJoyLeft);
  myRangeRight.add(myJoyRight);
}

// ----------------------------------------------------------------------------------------
// Both ways out of eMinMax - the button and the timeout - store what has been measured. The
// on-screen prompt is "Press button to exit", and an early exit still carries a full set of center
// values plus whatever travel the user swept. If an axis was not accepted, the result screen
// says which one before the calibration closes.
void JoystickCalibration::finish()
{
  myResultLeft  = applyCalibration(myJoyLeft,  myRangeLeft,  InputController::cNamespaceJoyL);
  myResultRight = applyCalibration(myJoyRight, myRangeRight, InputController::cNamespaceJoyR);

  if (myResultLeft.allAccepted() && myResultRight.allAccepted())
  {
    myPhase = EPhase::eIdle;
    setState(EState::eNotActive);
    return;
  }

  myPhase           = EPhase::eResult;
  myStopRequested   = false;   // the press that ended eMinMax does not close the result screen
  myResultStartTime = millis();
  markRedraw();
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

    case EPhase::eCenter:
      drawCenterScreen(forceRedraw);
      break;

    case EPhase::eMinMax:
      drawMinMaxScreen(forceRedraw);
      break;

    case EPhase::eResult:
      drawResultScreen(forceRedraw);
      break;
  }

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawCenterScreen(bool forceRedraw)
{
  const bool both = myJoyLeft.isActive() && myJoyRight.isActive();

  if (forceRedraw == true || needsRedraw() == true)
  {
    const ReleaseStep& step = cReleaseSteps[myReleaseStep];
    const String prompt = String(step.axis == 2 ? "Twist" : "Push")
                        + (both ? " the joysticks " : " the joystick ") + step.prompt;

    myUI.tft().fillScreen(TFT_BLACK);
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString("Center Position Calibration", myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
    myUI.tft().drawString(prompt, myUI.width()/2, 80, 4);
    myUI.tft().drawString("and let go.", myUI.width()/2, 110, 4);

    // ----- Progress bar: the steps done
    myUI.tft().drawSmoothRoundRect(100-cRectR, 170-cRectWidth/2, cRectR, cRectR-1, myUI.width()-200+2*cRectR, cRectWidth, cBorderCol, TFT_BLACK);
    if (myReleaseStep > 0)
      myUI.tft().drawWideLine(100, 170, 100+float(myUI.width()-200)*myReleaseStep/cNumReleaseSteps, 170, cRectWidth-6, TFT_BLUE, TFT_BLUE);
  }

  if (forceRedraw == true || needsRedraw() == true || needsPartialRedraw() == true)
  {
    int y = 210;
    if (myJoyLeft.isActive())
    {
      drawReleaseStatus(both ? "Left joystick: " : "Joystick: ", myReleaseLeft, y);
      y += 30;
    }
    if (myJoyRight.isActive())
      drawReleaseStatus("Right joystick: ", myReleaseRight, y);
  }
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::drawReleaseStatus(const char* label, const CenterRelease& release, int y)
{
  const char* status = "push";
  switch (release.state)
  {
    case ERelease::ePush:    status = "push";       break;
    case ERelease::eLetGo:   status = "let go";     break;
    case ERelease::eSettle:
    case ERelease::eMeasure: status = "hold still"; break;
    case ERelease::eDone:    status = "done";       break;
  }

  myUI.tft().setTextDatum(TC_DATUM);
  myUI.tft().setTextColor(release.state == ERelease::eDone ? cGreenCol : TFT_WHITE, TFT_BLACK);
  myUI.tft().setTextPadding(myUI.tft().textWidth("Right joystick: hold still", 4)); // blanks the previous text
  myUI.tft().drawString(String(label) + status, myUI.width()/2, y, 4);
  myUI.tft().setTextPadding(0);
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
    myUI.tft().drawString("Move the joysticks on all axes to", myUI.width()/2, 80, 4);
    myUI.tft().drawString("their min/max positions.", myUI.width()/2, 110, 4);
    myUI.tft().drawString("Press button to exit.", myUI.width()/2, 200, 4);
  }

  // ----- Progress bar
  myUI.tft().drawSmoothRoundRect(100-cRectR, 170-cRectWidth/2, cRectR, cRectR-1, myUI.width()-200+2*cRectR, cRectWidth, cBorderCol, TFT_BLACK);
  myUI.tft().drawWideLine(100, 170,
                                  100+min(float(myUI.width()-200)/cMinMaxDuration*(millis()-myCalibrationStartTime),
                                          float(myUI.width()-200)),
                                  170, cRectWidth-6, TFT_BLUE, TFT_BLUE);

  // ----- Left joystick
  const RangeTracker<3>& l = myRangeLeft;
  drawSliderH(cLeftXPosX, cLeftXPosY, l.min(0), l.max(0), l.center(0), myJoyLeft.readRaw(0), myJoyLeft.isActive());
  drawSliderV(cLeftYPosX, cLeftYPosY, l.min(1), l.max(1), l.center(1), myJoyLeft.readRaw(1), myJoyLeft.isActive());
  drawSliderA(cLeftZPosX, cLeftZPosY, l.min(2), l.max(2), l.center(2), myJoyLeft.readRaw(2), myJoyLeft.isActive());

  // ----- Right joystick
  const RangeTracker<3>& r = myRangeRight;
  drawSliderH(cRightXPosX, cRightXPosY, r.min(0), r.max(0), r.center(0), myJoyRight.readRaw(0), myJoyRight.isActive());
  drawSliderV(cRightYPosX, cRightYPosY, r.min(1), r.max(1), r.center(1), myJoyRight.readRaw(1), myJoyRight.isActive());
  drawSliderA(cRightZPosX, cRightZPosY, r.min(2), r.max(2), r.center(2), myJoyRight.readRaw(2), myJoyRight.isActive());
}

// ----------------------------------------------------------------------------------------
// Static screen: drawn once, then left alone until step() closes it.
void JoystickCalibration::drawResultScreen(bool forceRedraw)
{
  if (forceRedraw == false && needsRedraw() == false)
    return;

  myUI.tft().fillScreen(TFT_BLACK);

  myUI.tft().setTextDatum(TC_DATUM);
  myUI.tft().setTextColor(TFT_WHITE);
  myUI.tft().drawString("Min/Max Calibration", myUI.width()/2, 20, 4);
  myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
  myUI.tft().drawString("Not moved during calibration:", myUI.width()/2, 80, 4);

  int y = 110;
  myUI.tft().setTextColor(TFT_ORANGE);
  if (myResultLeft.allAccepted() == false)
  {
    myUI.tft().drawString("Left joystick: " + rejectedAxes(myResultLeft), myUI.width()/2, y, 4);
    y += 30;
  }
  if (myResultRight.allAccepted() == false)
    myUI.tft().drawString("Right joystick: " + rejectedAxes(myResultRight), myUI.width()/2, y, 4);

  myUI.tft().setTextColor(TFT_WHITE);
  myUI.tft().drawString("Full range stored for these axes.", myUI.width()/2, 170, 4);
  myUI.tft().drawString("Press button to exit.", myUI.width()/2, 200, 4);
}

// ----------------------------------------------------------------------------------------
void JoystickCalibration::start()
{
  myReleaseLeft  = CenterRelease();
  myReleaseRight = CenterRelease();
  myReleaseStep  = 0;
  startReleaseStep();

  myPhase         = EPhase::eCenter;
  myStopRequested = false;
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
    myUI.tft().drawWideLine(x+cRectLength/2+mapf(max, center, cADC_MAX, 0, cRectLength/2), y,
                               x+mapf(min, cADC_MIN, center, 0, cRectLength/2), y, cRectWidth-6, TFT_BLUE, TFT_BLUE);

    // Dynamic visualization of current axis position
    if (raw <= center)
      myUI.tft().drawSpot(x+mapf(raw, cADC_MIN, center, 0, cRectLength/2), y, cSpotR, TFT_NAVY, TFT_NAVY);
    else
      myUI.tft().drawSpot(x+cRectLength/2+mapf(raw, center, cADC_MAX, 0, cRectLength/2), y, cSpotR, TFT_NAVY, TFT_NAVY);
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
    myUI.tft().drawWideLine(x, y+cRectLength/2-mapf(max, center, cADC_MAX, 0, cRectLength/2),
                               x, y+cRectLength-mapf(min, cADC_MIN, center, 0, cRectLength/2), cRectWidth-6, TFT_BLUE, TFT_BLUE);

    // Dynamic visualization of current axis position
    if (raw <= center)
      myUI.tft().drawSpot(x, y+cRectLength-mapf(raw, cADC_MIN, center, 0, cRectLength/2), cSpotR, TFT_NAVY, TFT_NAVY);
    else
      myUI.tft().drawSpot(x, y+cRectLength/2-mapf(raw, center, cADC_MAX, 0, cRectLength/2), cSpotR, TFT_NAVY, TFT_NAVY);
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
                                mapf(min, cADC_MIN, center, 185, 360),
                                mapf(max, center, cADC_MAX, 0, 175), TFT_BLUE, TFT_BLUE, true);

    // Dynamic visualization of current axis position
    if (raw <= center)
    {
      float sx = -sinf((mapf(raw, cADC_MIN, center, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf((mapf(raw, cADC_MIN, center, 185, 360))*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myUI.tft().drawSpot(sx, sy, cSpotR, TFT_NAVY, TFT_NAVY);
    }
    else
    {
      float sx = -sinf(mapf(raw, center, cADC_MAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + x;
      float sy = +cosf(mapf(raw, center, cADC_MAX, 0, 175)*deg2rad)*(cArcR+cArcIR)/2.0f + y;
      myUI.tft().drawSpot(sx, sy, cSpotR, TFT_NAVY, TFT_NAVY);
    }
  }

  // Mid point indicator
  myUI.tft().drawWideLine(x, y+cArcIR, x, y+cArcR, 1, active ? cBorderCol : cBorderColNA);
}


