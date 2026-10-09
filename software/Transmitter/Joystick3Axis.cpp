#include <Preferences.h>
#include <Streaming.h>
#include "Joystick3Axis.h"
#include "Utilities.h"

// ---- Joystick3Axis ---------------------------------------------------------------------

namespace
{

// cDeadzone is the distance that needs to be moved before it interacts
const int cDeadzone = 200;     

// Threshold values to trigger up, down, left and right 
const int cDirOnThreshold = 500;
const int cDirOffThreshold = 300;

// Auto repeat related constants
const unsigned long cAutoRepeatDelay = 1500;
const unsigned long cRepeatInterval  = 300;

} // namespace

// ----------------------------------------------------------------------------------------
Joystick3Axis::Joystick3Axis(int pinX, int pinY, int pinZ)
: myPinX(pinX), myPinY(pinY), myPinZ(pinZ)
{}

// ----------------------------------------------------------------------------------------
void Joystick3Axis::begin()
{
  // Runs for every joystick, active or not: the calibration members hold usable values whatever
  // myIsActive is set to, so enabling a stick at runtime cannot expose values nothing has set.
  //
  // Opened read-only: a stick that has never been calibrated has no namespace, and a read-write
  // open would create an empty one. Read-only leaves NVS untouched and getInt() then returns the
  // defaults passed below.
  Preferences p;
  String ns = "Joystick" + String(myPinX);
  p.begin(ns.c_str(), true);

  setAxisCalibration(myMinX, myMaxX, myCenterX,
                     p.getInt("minX",    cADC_MIN),
                     p.getInt("maxX",    cADC_MAX),
                     p.getInt("centerX", cADC_MAX/2), "X");

  setAxisCalibration(myMinY, myMaxY, myCenterY,
                     p.getInt("minY",    cADC_MIN),
                     p.getInt("maxY",    cADC_MAX),
                     p.getInt("centerY", cADC_MAX/2), "Y");

  setAxisCalibration(myMinZ, myMaxZ, myCenterZ,
                     p.getInt("minZ",    cADC_MIN),
                     p.getInt("maxZ",    cADC_MAX),
                     p.getInt("centerZ", cADC_MAX/2), "Z");

  p.end();
}

// ----------------------------------------------------------------------------------------
void Joystick3Axis::setCalibrationData(int minX, int maxX, int centerX,
                                       int minY, int maxY, int centerY,
                                       int minZ, int maxZ, int centerZ)
{
  if (myIsActive == false)
    return;

  setAxisCalibration(myMinX, myMaxX, myCenterX, minX, maxX, centerX, "X");
  setAxisCalibration(myMinY, myMaxY, myCenterY, minY, maxY, centerY, "Y");
  setAxisCalibration(myMinZ, myMaxZ, myCenterZ, minZ, maxZ, centerZ, "Z");

  // Store calibration data to EEPROM. The members rather than the parameters, so a rejected
  // axis persists the defaults it fell back to instead of the data that was turned down.
  Preferences p;
  String ns = "Joystick" + String(myPinX);
  p.begin(ns.c_str());
 
  p.putInt("minX",    myMinX);
  p.putInt("maxX",    myMaxX);
  p.putInt("centerX", myCenterX);

  p.putInt("minY",    myMinY);
  p.putInt("maxY",    myMaxY);
  p.putInt("centerY", myCenterY);

  p.putInt("minZ",    myMinZ);
  p.putInt("maxZ",    myMaxZ);
  p.putInt("centerZ", myCenterZ);

  p.end();
}

// ----------------------------------------------------------------------------------------
void Joystick3Axis::setAxisCalibration(int& min, int& max, int& center,
                                       int newMin, int newMax, int newCenter, const char* axis)
{
  // Both halves of the travel have to clear the deadzone with room left over. Below that the
  // getters either divide by a span that is zero or negative, or map a range that the deadzone
  // has already swallowed - and the result is a stuck full-deflection reading rather than an
  // inaccurate one. An axis the user never moved during the min/max sweep arrives here with all
  // three values equal, which is the case this mainly catches.
  if (newMin + 2*cDeadzone < newCenter && newCenter + 2*cDeadzone < newMax)
  {
    min    = newMin;
    max    = newMax;
    center = newCenter;
    return;
  }

  min    = cADC_MIN;
  max    = cADC_MAX;
  center = cADC_MAX/2;

  Serial << __PRETTY_FUNCTION__ << " -> joystick on pin " << myPinX << ", axis " << axis
         << ": min " << newMin << " max " << newMax << " center " << newCenter
         << " leaves no usable travel -> using defaults" << endl;
}

// ----------------------------------------------------------------------------------------
void Joystick3Axis::update()
{
  myValueX = readAxis(myPinX, myMinX, myMaxX, myCenterX);
  myValueY = readAxis(myPinY, myMinY, myMaxY, myCenterY);
  myValueZ = readAxis(myPinZ, myMinZ, myMaxZ, myCenterZ);

  // All four run on every update. updateDirectionState() is a state machine with timestamps,
  // not a query, so a direction that is skipped keeps a stale activation time and can fire an
  // immediate auto-repeat when it is next consulted.
  myIsUp    = updateDirectionState( myValueY, myUpState);
  myIsDown  = updateDirectionState(-myValueY, myDownState);
  myIsRight = updateDirectionState( myValueX, myRightState);
  myIsLeft  = updateDirectionState(-myValueX, myLeftState);
}

// ----------------------------------------------------------------------------------------
uint16_t Joystick3Axis::readRaw(int pin) const
{
  if (myIsActive == false)
    return 0;

  return analogRead(pin);
}

// ----------------------------------------------------------------------------------------
int Joystick3Axis::readAxis(int pin, int min, int max, int center) const
{
  if (myIsActive == false)
    return 0;

  // Oversampling (e.g. 8 samples) might reduce noise -> not required
  uint16_t value = analogRead(pin);

  if (value >= center - cDeadzone && value <= center + cDeadzone)
    return 0;

  // Clamp to calibration range
  if (value > max) value = max;
  if (value < min) value = min;

  // Map to [cMIN, cMAX]. The clamp guarantees that range for any calibration data: it is the
  // getter's documented contract, and the value drives whatever the robot moves.
  const float mapped = (value > center)
                     ? mapf(value, center + cDeadzone, max, 0, cMAX)
                     : mapf(value, min, center - cDeadzone, cMIN, 0);

  return constrain(int(mapped), cMIN, cMAX);
}

// ----------------------------------------------------------------------------------------
// Shared on/off-threshold + auto-repeat state machine for the four joystick directions below.
// 'value' must already be signed so that a positive value means "this direction is currently
// active" - isDown()/isLeft() pass in the negated axis value to reuse the same comparisons
// isUp()/isRight() use directly.
bool Joystick3Axis::updateDirectionState(int value, DirectionState& state)
{
  // Below threshold -> direction "key" is not active
  if (value < cDirOffThreshold)
  {
    state.myIsActive = false;
    state.myIsRepeatActive = false;
    return false;
  }

  // State change from inactive to active? -> direction "key" is active?
  if (state.myIsActive == false && value > cDirOnThreshold)
  {
    state.myIsActive = true;
    state.myTimeActive = millis();
    return true;
  }

  // State change from active to start auto repeat?
  if (state.myIsActive == true && state.myIsRepeatActive == false && millis()-state.myTimeActive > cAutoRepeatDelay)
  {
    state.myIsRepeatActive = true;
    state.myTimeRepeatTriggered = millis();
    return true;
  }

  // Auto repeat already active, trigger another direction "key"?
  if (state.myIsActive == true && state.myIsRepeatActive == true && millis()-state.myTimeRepeatTriggered > cRepeatInterval)
  {
    state.myTimeRepeatTriggered = millis();
    return true;
  }

  return false;
}


