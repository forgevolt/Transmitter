#include <TFT_eSPI.h>

#include <Streaming.h>
#include "JoystickCalibration.h"
#include <VectorUtilities.h>
using namespace vectorutilities;
using namespace joystickutilities;


// ---- JoystickCalibration ---------------------------------------------------------------

namespace
{

// ----- Timing

// The center is measured after releases from six directions. Measured on both transmitters: a
// released stick is still after 0.3 - 0.5 s, and at rest the noise is up to 8 counts in 100 ms.
const unsigned long cStillDuration   = 200; // every axis within cStillCounts for this long (ms) ...
const int           cStillCounts     = 15;  // ... counts as holding still
const unsigned long cMeasureDuration = 300; // then the rest position is averaged for this long (ms)

// Total of 10 seconds to move all axes (up to six) to min/max positions
const unsigned long cMinMaxDuration = 10000;

// How long the list of axes that were not accepted stays on screen (unless the button is pressed)
const unsigned long cResultDuration = 5000;

// ----- The six release steps of eCenter

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
  { 1, +1, "Push up and let go" },          { 1, -1, "Push down and let go" },
  { 0, -1, "Push to the left and let go" }, { 0, +1, "Push to the right and let go" },
  { 2,  0, "Twist either way and let go" }, { 2,  0, "Twist the other way and let go" }
};

const int cNumReleaseSteps = sizeof(cReleaseSteps) / sizeof(cReleaseSteps[0]);

// ----- Layout of eCenter

// Joystick drawings of eCenter: one per joystick, placed like the joysticks themselves. Each is a
// sprite of 2*cStickHalf pixels, centered at x and cStickY.
const int   cStickY       = 190;
const int   cStickHalf    = 72;
const int   cStickR       = 44;     // circle of the joystick
const int   cStickDotR    = 6;      // the dot that follows the joystick ...
const int   cStickTravel  = cStickR - cStickDotR - 2;   // ... this far from the center at full deflection
const float cArrowLength  = cStickR + 20;
const float cArrowWidth   = 7;
const float cArrowHead    = 15;
const float cTwistR       = cStickR + 14;               // the twist arrow runs along this radius
const int   cStatusY      = cStickY + cStickHalf + 4;   // the word below the drawing
const int   cProgressY    = 306;                        // the six dots of the steps

// ----- Layout of eMinMax

// Raw range of an uncalibrated axis; the sliders are drawn against it
const int cADC_MIN = 0;
const int cADC_MAX = AnalogConfig().adcMax;

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

// ----- Drawing helpers of eCenter

// Angles as TFT_eSPI's smooth arcs count them: 0 at the bottom, clockwise. A positive twist turns
// clockwise, as on the main screen.
float arcX(float angle, float r) { return cStickHalf - sinf(angle*deg2rad)*r; }
float arcY(float angle, float r) { return cStickHalf + cosf(angle*deg2rad)*r; }

// A filled triangle with anti-aliased edges, to match the smooth lines and arcs next to it.
// TFT_eSPI's fillTriangle() has hard edges, so each pixel's coverage is computed here from its
// distance to the three edges, and the edge pixels are blended with what is already in the sprite.
void fillSmoothTriangle(TFT_eSprite& s, float x0, float y0, float x1, float y1, float x2, float y2,
                        uint32_t color)
{
  // Corners in the order that puts the inside on the positive side of every edge
  if ((x1 - x0)*(y2 - y0) - (x2 - x0)*(y1 - y0) < 0)
  {
    const float tx = x1, ty = y1;
    x1 = x2;  y1 = y2;
    x2 = tx;  y2 = ty;
  }

  // Edge i as nx*x + ny*y + c: the distance in pixels, positive inside
  const float px[3] = { x0, x1, x2 }, py[3] = { y0, y1, y2 };
  float nx[3], ny[3], c[3];
  for (int i = 0; i < 3; i++)
  {
    const int   j   = (i + 1) % 3;
    const float dx  = px[j] - px[i], dy = py[j] - py[i];
    const float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.5f)
      return;
    nx[i] = -dy/len;
    ny[i] =  dx/len;
    c[i]  = -(nx[i]*px[i] + ny[i]*py[i]);
  }

  const int xMin = int(floorf(fminf(x0, fminf(x1, x2)))), xMax = int(ceilf(fmaxf(x0, fmaxf(x1, x2))));
  const int yMin = int(floorf(fminf(y0, fminf(y1, y2)))), yMax = int(ceilf(fmaxf(y0, fmaxf(y1, y2))));

  for (int y = yMin; y <= yMax; y++)
    for (int x = xMin; x <= xMax; x++)
    {
      float d = 1e9f;
      for (int i = 0; i < 3; i++)
        d = fminf(d, nx[i]*x + ny[i]*y + c[i]);

      const float cover = d + 0.5f;   // fraction of the pixel inside
      if (cover >= 1)
        s.drawPixel(x, y, color);
      else if (cover > 0)
        s.drawPixel(x, y, s.alphaBlend(uint8_t(cover*255), uint16_t(color), s.readPixel(x, y)));
    }
}

// A straight arrow from near the center outwards; (dx, dy) is the direction on the screen.
void drawArrow(TFT_eSprite& s, float dx, float dy, uint32_t color)
{
  const float c = cStickHalf;
  const float shaft = cArrowLength - cArrowHead*0.8f;
  s.drawWideLine(c + dx*12, c + dy*12, c + dx*shaft, c + dy*shaft, cArrowWidth, color, TFT_BLACK);

  const float base = cArrowLength - cArrowHead;
  const float half = cArrowHead*0.7f;
  fillSmoothTriangle(s, c + dx*cArrowLength, c + dy*cArrowLength,
                        c + dx*base - dy*half, c + dy*base + dx*half,
                        c + dx*base + dy*half, c + dy*base - dx*half, color);
}

// One arrowhead of the twist arrow, its base across the arc at 'angle'. The tip lies on the arc
// itself, so the head follows the curve; direction +1 points clockwise, -1 counterclockwise.
void drawTwistHead(TFT_eSprite& s, float angle, int direction, uint32_t color)
{
  const float tip  = angle + direction * cArrowHead / cTwistR / deg2rad;
  const float half = cArrowHead*0.6f;
  fillSmoothTriangle(s, arcX(tip, cTwistR), arcY(tip, cTwistR),
                        arcX(angle, cTwistR + half), arcY(angle, cTwistR + half),
                        arcX(angle, cTwistR - half), arcY(angle, cTwistR - half), color);
}

// The twist arrow over the top of the circle. heads: 0 both ends, +1 clockwise, -1 counterclockwise.
// Each head starts a degree before the end of the arc, so it covers the arc's square end.
void drawTwistArrow(TFT_eSprite& s, int heads, uint32_t color)
{
  const uint32_t from = 115, to = 245;

  // TFT_eSPI's arc reaches about half a pixel beyond r and ir, so +-3 gives a band 7 pixels wide
  // (as wide as the straight arrows), centered on cTwistR like the heads
  const int32_t r = int32_t(cTwistR) + 3, ir = int32_t(cTwistR) - 3;
  s.drawSmoothArc(cStickHalf, cStickHalf, r, ir, from, to, color, TFT_BLACK, false);
  if (heads >= 0) drawTwistHead(s, to - 1.0f,   +1, color);
  if (heads <= 0) drawTwistHead(s, from + 1.0f, -1, color);
}

void drawCheck(TFT_eSprite& s, uint32_t color)
{
  const float c = cStickHalf;
  s.drawWideLine(c - 18, c + 2,  c - 6,  c + 14, cArrowWidth, color, TFT_BLACK);
  s.drawWideLine(c - 6,  c + 14, c + 20, c - 14, cArrowWidth, color, TFT_BLACK);
}

// ----- Storing the result

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
    myUI.tft().fillScreen(TFT_BLACK);
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString("Center Position Calibration", myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
    myUI.tft().drawString(cReleaseSteps[myReleaseStep].prompt, myUI.width()/2, 70, 4);

    if (both)
    {
      myUI.tft().setTextColor(cBorderCol);
      myUI.tft().drawString("Left",  130, 100, 2);
      myUI.tft().drawString("Right", 350, 100, 2);
    }

    // ----- The steps: done ones green
    for (int i = 0; i < cNumReleaseSteps; i++)
    {
      const float x = myUI.width()/2 + (i - (cNumReleaseSteps-1)/2.0f)*22;
      if (i < myReleaseStep)
        myUI.tft().fillSmoothCircle(x, cProgressY, 6, cGreenCol, TFT_BLACK);
      else
        myUI.tft().drawSmoothCircle(x, cProgressY, 6, cBorderCol, TFT_BLACK);
    }

    myViewLeft.valid = myViewRight.valid = false;
  }

  if (both)
  {
    drawStick(130, myJoyLeft,  myReleaseLeft,  myViewLeft);
    drawStick(350, myJoyRight, myReleaseRight, myViewRight);
  }
  else if (myJoyLeft.isActive())
    drawStick(myUI.width()/2, myJoyLeft, myReleaseLeft, myViewLeft);
  else if (myJoyRight.isActive())
    drawStick(myUI.width()/2, myJoyRight, myReleaseRight, myViewRight);
}

// ----------------------------------------------------------------------------------------
// One joystick: the circle, an arrow in the step's direction in the color of its state (a check
// mark once done), where the stick is - x/y as a dot in the push steps, the twist on the circle in
// the twist steps - and the state in words below. Drawn only when something visible has changed.
void JoystickCalibration::drawStick(int x, const InputController::Joystick& stick,
                                    const CenterRelease& release, StickView& view)
{
  const ReleaseStep& step = cReleaseSteps[myReleaseStep];
  const bool twisting = (step.axis == 2);

  StickView now;
  now.valid = true;
  now.state = release.state;
  if (twisting)
  {
    const float twist = stick.getZ() * 175.0f / cMAX;   // degrees, as on the main screen
    now.twistX = lroundf(arcX(twist, cStickR));
    now.twistY = lroundf(arcY(twist, cStickR));
  }
  else
  {
    now.dotX = cStickHalf + stick.getX() * cStickTravel / cMAX;
    now.dotY = cStickHalf - stick.getY() * cStickTravel / cMAX;
  }

  if (view.valid && view.state == now.state && view.dotX == now.dotX && view.dotY == now.dotY &&
      view.twistX == now.twistX && view.twistY == now.twistY)
    return;

  const char* word  = "push";
  uint32_t    color = TFT_WHITE;
  switch (now.state)
  {
    case ERelease::ePush:    word = "push";       color = TFT_WHITE;  break;
    case ERelease::eLetGo:   word = "let go";     color = cBlueCol;   break;
    case ERelease::eSettle:
    case ERelease::eMeasure: word = "hold still"; color = TFT_ORANGE; break;
    case ERelease::eDone:    word = "done";       color = cGreenCol;  break;
  }

  // ----- The state in words, when it has changed
  if (view.valid == false || view.state != now.state)
  {
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(color, TFT_BLACK);
    myUI.tft().setTextPadding(myUI.tft().textWidth("hold still", 4)); // blanks the previous word
    myUI.tft().drawString(word, x, cStatusY, 4);
    myUI.tft().setTextPadding(0);
  }

  view = now;

  // ----- The drawing
  static bool reported = false;
  if (createBuffer(2*cStickHalf, 2*cStickHalf, __PRETTY_FUNCTION__, reported) == false)
    return;

  myBuf.fillSprite(TFT_BLACK);
  myBuf.drawSmoothCircle(cStickHalf, cStickHalf, cStickR, cBorderCol, TFT_BLACK);
  myBuf.fillSmoothCircle(cStickHalf, cStickHalf, 3, cBorderCol, TFT_BLACK);

  if (now.state == ERelease::eDone)
    drawCheck(myBuf, color);
  else if (step.axis == 0)
    drawArrow(myBuf, float(step.sign), 0, color);
  else if (step.axis == 1)
    drawArrow(myBuf, 0, -float(step.sign), color);
  else if (release.twistSign != 0)          // the second twist: the other way
    drawTwistArrow(myBuf, -release.twistSign, color);
  else if (now.state == ERelease::ePush)    // the first twist: either way ...
    drawTwistArrow(myBuf, 0, color);
  else                                      // ... until it was pushed
    drawTwistArrow(myBuf, release.pushSign, color);

  // ----- Where the stick is
  if (twisting)
    myBuf.fillSmoothCircle(now.twistX, now.twistY, 5, TFT_WHITE, TFT_BLACK);
  else
  {
    myBuf.fillSmoothCircle(now.dotX, now.dotY, cStickDotR + 2, TFT_BLACK, TFT_BLACK);
    myBuf.fillSmoothCircle(now.dotX, now.dotY, cStickDotR, TFT_WHITE, TFT_BLACK);
  }

  myBuf.pushSprite(x - cStickHalf, cStickY - cStickHalf);
  myBuf.deleteSprite();
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


