#include <TFT_eSPI.h>
#include "IMUCalibration.h"
#include <Preferences.h>


// ---- IMUCalibration --------------------------------------------------------------------

namespace
{

const unsigned long cWaitDuration  = 3000; // wait time until the calibration begins
const String cMenuName = "IMU Calibration";

} // namespace

// ----------------------------------------------------------------------------------------
IMUCalibration::IMUCalibration(UserInterface& ui, Menu& menu, MPU6050& imu)
: MenuItem(ui, menu, cMenuName),
  myIMU(imu)
{
  // Read calibration data from EEPROM or use default values
  Preferences p;
  p.begin("IMU");
    myIMU.setGyroOffsets(p.getFloat("gyroXoffset", 0),
                         p.getFloat("gyroYoffset", 0), 
                         p.getFloat("gyroZoffset", 0));
    myIMU.setAccOffsets(p.getFloat("accXoffset", 0),
                        p.getFloat("accYoffset", 0), 
                        p.getFloat("accZoffset", 0));   
  p.end();
}

// ----------------------------------------------------------------------------------------
void IMUCalibration::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  // Has the IMU calibration wait time expired?
  if (myIsCalibrationWaitPeriodActive == true && millis()-myCalibrationStartTime > cWaitDuration)
  {
    myIsCalibrationWaitPeriodActive = false;
    markPartialRedraw();
  }
  else if (myIsCalibrationWaitPeriodActive == false)
  {
    // Deliberate exception to the "step() must not block" rule the rest of this codebase
    // follows: calcGyroOffsets() blocks for several hundred synchronous I2C reads (well under a second in
    // practice). While it runs, rendering pauses. Calibration is a rare, explicit user action, 
    // so this is accepted rather than rewritten to spread the 500 samples across multiple calls.
    myIMU.calcOffsets(true, true);

    // Make the offset values persistent
    Preferences p;
    p.begin("IMU");
      p.putFloat("gyroXoffset", myIMU.getGyroXoffset());
      p.putFloat("gyroYoffset", myIMU.getGyroYoffset());
      p.putFloat("gyroZoffset", myIMU.getGyroZoffset());

      p.putFloat("accXoffset", myIMU.getAccXoffset());
      p.putFloat("accYoffset", myIMU.getAccYoffset());
      p.putFloat("accZoffset", myIMU.getAccZoffset());
    p.end();      

    setState(EState::eNotActive);
  }
  else 
  {
    markPartialRedraw();
  }
}

// ----------------------------------------------------------------------------------------
void IMUCalibration::draw(bool forceRedraw)
{
  if (isActive() == false)
  {
    MenuItem::draw(forceRedraw);
    return;
  }

  if (forceRedraw == false && needsRedraw() == false && needsPartialRedraw() == false)
    return;

  if (forceRedraw == true || needsRedraw() == true)  
  {
    myUI.tft().fillScreen(TFT_BLACK);
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString(cMenuName, myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
    myUI.tft().drawString("Place transmitter on a flat surface!", myUI.width()/2, 80, 4);
  }

  if (myIsCalibrationWaitPeriodActive == true)
  {
    // Remaining time is computed in unsigned arithmetic, the same idiom step() uses, so it stays
    // correct across the millis() rollover. Only the finished value becomes a float.
    const unsigned long elapsed   = millis() - myCalibrationStartTime;
    const unsigned long remaining = (elapsed < cWaitDuration) ? cWaitDuration - elapsed : 0;

    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE, TFT_BLACK);
    myUI.tft().drawString("Calibration starts in ...", myUI.width()/2, 140, 4);
    myUI.tft().setTextPadding(myUI.tft().textWidth("00", 4)); // The padding will blank any area not covered by the string.
    myUI.tft().drawFloat(remaining/1000.0f, 0, myUI.width()/2, 170, 4);
    myUI.tft().setTextPadding(0);
  }
  else
  {
    myUI.tft().fillRect(0, 140, myUI.width(), 180, TFT_BLACK);
  }
  
  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void IMUCalibration::start()
{
  myIsCalibrationWaitPeriodActive = true;
  myCalibrationStartTime = millis();
  markRedraw();
  setState(EState::eActive);
}
