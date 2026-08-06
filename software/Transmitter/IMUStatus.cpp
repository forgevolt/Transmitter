#include "UserInterface.h"
#include "IMUStatus.h"
#include "Utilities.h"

// ---- IMUStatus -------------------------------------------------------------------------

constexpr float cRadius     = 25;    // Radius of the circle 
constexpr float cSpotR      = 5;     // Radius of the spot indication the current IMU values
constexpr float cPosX       = cTFTWidth/2; 
constexpr float cPosY       = 283;
constexpr float cAngleLimit = 45;
constexpr uint32_t cSpotCol = 0x4CBB;

constexpr unsigned long cTempIntervalMs = 15000;

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
  // Sensor Orientation: X-axis points right, Y-axis points toward the top of the transmitter.  
  float angleX = myIMU.getAngleX();
  float angleY = myIMU.getAngleY();
  float angleZ = myIMU.getAngleZ();

  // Visualize angles
  angleX = constrain(angleX, -cAngleLimit, +cAngleLimit);
  angleY = constrain(angleY, -cAngleLimit, +cAngleLimit);
  angleZ = constrain(angleZ, -cAngleLimit, +cAngleLimit);

  // Mapping from a square in the interval -1 to 1, for both x and y coordinates to a unit circle 
  //    y * sqrt(1 - x * x / 2) --> x
  //    x * sqrt(1 - y * y / 2) --> y

  float x = angleX / cAngleLimit; // convert to interval [-1, 1]
  float y = angleY / cAngleLimit;

  mySX = y * sqrtf(1 - x * x / 2) * (cRadius-cSpotR-1);
  mySY = x * sqrtf(1 - y * y / 2) * (cRadius-cSpotR-1);

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

  // ---- Temperature as measured by the MPU-6050
  myUI.tft().setTextDatum(TR_DATUM);
  myUI.tft().setTextColor(cBorderCol, cStatusAreaCol);
  myUI.tft().setTextPadding(myUI.tft().textWidth("000", 2)); // Padding will blank area not covered by the string
  myUI.tft().drawFloat(myTemp, 0, 95, 22, 2);
  myUI.tft().setTextPadding(0);
  myUI.tft().drawString("o", 101, 21, 1);

  // ---- IMU values
  static bool reported = false;
  if (createBuffer(int(cRadius*2+1), int(cRadius*2+1), __PRETTY_FUNCTION__, reported) == false)
    return;

  myBuf.fillSprite(TFT_TRANSPARENT);

  // Clear before redrawing
  myBuf.fillCircle(cRadius, cRadius, cRadius, cMainAreaCol);

  /* ---- Sensor fusion with an 6-axis IMU (MPU6050) for angle Z (yaw) is not good enough and drifts over time, 
          9-axis IMU is required -> disable
  myUI.tft().drawSpot(sinf(angleZ*deg2rad)*(cRadius+cSpotR+2) + cPosX, 
                         cPosY-cosf(angleZ*deg2rad)*(cRadius+cSpotR+2), 
                         cSpotR, cSpotCol);
  */

  // Sensor Orientation: X-axis points right, Y-axis points toward the top of the transmitter.  
  myBuf.drawSpot(mySX+cRadius, mySY+cRadius, cSpotR, cSpotCol);

  myBuf.drawSmoothCircle(cRadius, cRadius, cRadius, cBorderCol, TFT_BLACK);
  myBuf.drawWideLine(cRadius, 0+1, cRadius, 2*cRadius-1, 1, cBorderCol);
  myBuf.drawWideLine(0+1, cRadius, 2*cRadius-1, cRadius, 1, cBorderCol); 

  myBuf.pushSprite(cPosX-cRadius, cPosY-cRadius, TFT_TRANSPARENT);
  myBuf.deleteSprite();

}
