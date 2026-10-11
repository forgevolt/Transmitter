#include "UserInterface.h"
#include "IMUStatus.h"
#include <VectorUtilities.h>
using namespace vectorutilities;

// ---- IMUStatus -------------------------------------------------------------------------

namespace
{

constexpr float cRadius     = 25;    // Radius of the circle
constexpr float cSpotR      = 5;     // Radius of the spot indicating the current IMU values
constexpr float cPosX       = cTFTWidth/2;
constexpr float cPosY       = 283;
constexpr float cAngleLimit = 45;
constexpr uint32_t cSpotCol = cBlueCol;

constexpr unsigned long cTempIntervalMs = 15000;

} // namespace

// ----------------------------------------------------------------------------------------
IMUStatus::IMUStatus(UserInterface& ui, MPU6050& imu)
: DisplayObject(ui),
  myIMU(imu),
  mySX(0), mySY(0),
  myLastTempUpdate(millis()),
  myTemp(0),
  myTempValid(false)
{}

// ----------------------------------------------------------------------------------------
void IMUStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  // ---- Compute "spot" position inside the IMU circle
  float oldSX = mySX;
  float oldSY = mySY;

  // Note: IMU getters return cached values rather than triggering a direct hardware read.
  // Sensor orientation: X-axis points right, Y-axis points toward the top of the transmitter.
  float angleX = myIMU.getAngleX();
  float angleY = myIMU.getAngleY();
  float angleZ = myIMU.getAngleZ();

  // Visualize angles
  angleX = constrain(angleX, -cAngleLimit, +cAngleLimit);
  angleY = constrain(angleY, -cAngleLimit, +cAngleLimit);
  angleZ = constrain(angleZ, -cAngleLimit, +cAngleLimit);

  // Map the square [-1, 1] x [-1, 1] to the unit disk so that the spot stays inside the circle.
  // The axes are swapped on purpose: the spot's x follows angleY, its y follows angleX.
  const Vector2 spot = circularNormalization(angleY / cAngleLimit, angleX / cAngleLimit) * (cRadius-cSpotR-1);

  mySX = spot.x;
  mySY = spot.y;

  if (int(oldSX) != int(mySX) || int(oldSY) != int(mySY))
  {
    markRedraw();
  }

  // ---- Update the temperature periodically to avoid value "flickering"
  if (myTempValid == false || millis() - myLastTempUpdate > cTempIntervalMs)
  {
    myLastTempUpdate = millis();
    myTemp = myIMU.getTemp();
    myTempValid = true;
    markRedraw();
  }
}

// ----------------------------------------------------------------------------------------
void IMUStatus::draw(bool forceRedraw)
{
  if (isActive() == false || (forceRedraw == false && needsRedraw() == false))
    return;

  clearRedrawFlags();

  // ---- Board temperature
  // The MPU-6050 measures its own die, which sits on the PCB: the board temperature (plus a degree
  // or two from the chip's own heat), not the room.
  myUI.tft().setTextDatum(TR_DATUM);
  myUI.tft().setTextColor(cBorderCol, cStatusAreaCol);
  myUI.tft().setTextPadding(myUI.tft().textWidth("000", 2)); // The padding will blank any area not covered by the string.
  myUI.tft().drawFloat(myTemp, 0, 95, 22, 2);
  myUI.tft().setTextPadding(0);
  myUI.tft().drawString("o", 101, 21, 1);
  myUI.tft().setTextDatum(TL_DATUM);
  myUI.tft().drawString("C", 102, 22, 2);

  // ---- IMU values
  static bool reported = false;
  if (createBuffer(int(cRadius*2+1), int(cRadius*2+1), __PRETTY_FUNCTION__, reported) == false)
    return;

  myBuf.fillSprite(TFT_TRANSPARENT);

  // Clear before redrawing
  myBuf.fillCircle(cRadius, cRadius, cRadius, cMainAreaCol);

  /* ---- Yaw (angle Z) from a 6-axis IMU (MPU-6050) drifts over time; showing it would need a
          9-axis IMU, so it is disabled.
  myUI.tft().drawSpot(sinf(angleZ*deg2rad)*(cRadius+cSpotR+2) + cPosX,
                         cPosY-cosf(angleZ*deg2rad)*(cRadius+cSpotR+2),
                         cSpotR, cSpotCol);
  */

  // Sensor orientation: X-axis points right, Y-axis points toward the top of the transmitter.
  myBuf.drawSpot(mySX+cRadius, mySY+cRadius, cSpotR, cSpotCol);

  myBuf.drawSmoothCircle(cRadius, cRadius, cRadius, cBorderCol, TFT_BLACK);
  myBuf.drawWideLine(cRadius, 0+1, cRadius, 2*cRadius-1, 1, cBorderCol);
  myBuf.drawWideLine(0+1, cRadius, 2*cRadius-1, cRadius, 1, cBorderCol);

  myBuf.pushSprite(cPosX-cRadius, cPosY-cRadius, TFT_TRANSPARENT);
  myBuf.deleteSprite();

}
