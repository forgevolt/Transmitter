#include <TFT_eSPI.h>
#include "IMUCalibration.h"
#include <Preferences.h>
#include <Streaming.h>


// ---- IMUCalibration --------------------------------------------------------------------

namespace
{

const unsigned long cWaitDuration  = 3000; // wait time until the calibration begins
const String cMenuName = "IMU Calibration";

constexpr unsigned long cStoredDuration = 1500; // how long "Calibration stored" stays on screen
constexpr unsigned long cFailedDuration = 3000; // how long "Calibration failed" stays on screen

// Samples averaged into the offsets (as many as MPU6050_light's calcOffsets() takes), and how many
// are read per step(). 20 reads take about 8 ms at 400 kHz, so the measurement spreads over about
// 25 frames, and loop() keeps sending in between.
constexpr int cNumSamples     = CALIB_OFFSET_NB_MES;
constexpr int cSamplesPerStep = 20;

// The check after the measurement: with the new offsets, the averaged readings of the flat
// transmitter at rest must be close to (0, 0, 1 g) and (0, 0, 0). Otherwise it was moved, or
// MPU6050_light no longer works as assumed in step(), and the previous offsets are kept.
constexpr int   cCheckSamples = 20;
constexpr float cMaxAccError  = 0.05f; // g
constexpr float cMaxGyroError = 1.0f;  // deg/s

} // namespace

// ----------------------------------------------------------------------------------------
IMUCalibration::IMUCalibration(UserInterface& ui, Menu& menu, MPU6050& imu)
: MenuItem(ui, menu, cMenuName),
  myIMU(imu)
{
  // Read the calibration data from NVS, or use the defaults
  Preferences p;
  p.begin("IMU", true); // read-only: does not create the namespace
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

  // ---- Countdown
  if (myPhase == EPhase::eCountdown)
  {
    if (millis() - myPhaseStartTime > cWaitDuration)
    {
      myPhase = EPhase::eMeasuring;
      mySampleCount = 0;
      for (float& sum : mySum)
        sum = 0.0f;
    }

    markPartialRedraw();
    return;
  }

  // ---- Result on screen
  if (myPhase == EPhase::eStored || myPhase == EPhase::eFailed)
  {
    const unsigned long duration = (myPhase == EPhase::eStored) ? cStoredDuration : cFailedDuration;
    if (millis() - myPhaseStartTime > duration)
      setState(EState::eNotActive);

    return;
  }

  // ---- Measurement
  // Mirrors calcOffsets() of MPU6050_light 1.2.1 (MPU6050_light.cpp:138-165), spread over several
  // step() calls instead of one blocking loop of about half a second, so the link to the receiver
  // keeps running. It relies on three details of that implementation - re-check them when
  // updating the library:
  //
  // - fetchData() subtracts the offsets, so getter + offset is the raw reading. The offsets in
  //   use therefore stay untouched for the imu.update() calls in between.
  // - The Z acceleration offset is measured against 1 g, the transmitter lying flat.
  // - fetchData() only reads the sensor and leaves the angle filter of update() alone.
  //
  // applyAndCheckOffsets() catches a result that no longer fits.
  for (int i = 0; i < cSamplesPerStep && mySampleCount < cNumSamples; ++i, ++mySampleCount)
  {
    myIMU.fetchData();

    mySum[0] += myIMU.getAccX()  + myIMU.getAccXoffset();
    mySum[1] += myIMU.getAccY()  + myIMU.getAccYoffset();
    mySum[2] += myIMU.getAccZ()  + myIMU.getAccZoffset() - 1.0f;
    mySum[3] += myIMU.getGyroX() + myIMU.getGyroXoffset();
    mySum[4] += myIMU.getGyroY() + myIMU.getGyroYoffset();
    mySum[5] += myIMU.getGyroZ() + myIMU.getGyroZoffset();
  }

  markPartialRedraw();

  if (mySampleCount < cNumSamples)
    return;

  // ---- Result
  if (applyAndCheckOffsets() == false)
  {
    myPhase = EPhase::eFailed;
    myPhaseStartTime = millis();
    markPartialRedraw();
    return;
  }

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

  myPhase = EPhase::eStored;
  myPhaseStartTime = millis();
  markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
bool IMUCalibration::applyAndCheckOffsets()
{
  const float oldAcc[3]  = { myIMU.getAccXoffset(),  myIMU.getAccYoffset(),  myIMU.getAccZoffset()  };
  const float oldGyro[3] = { myIMU.getGyroXoffset(), myIMU.getGyroYoffset(), myIMU.getGyroZoffset() };

  myIMU.setAccOffsets (mySum[0] / cNumSamples, mySum[1] / cNumSamples, mySum[2] / cNumSamples);
  myIMU.setGyroOffsets(mySum[3] / cNumSamples, mySum[4] / cNumSamples, mySum[5] / cNumSamples);

  float check[6] = {};
  for (int i = 0; i < cCheckSamples; ++i)
  {
    myIMU.fetchData();

    check[0] += myIMU.getAccX();
    check[1] += myIMU.getAccY();
    check[2] += myIMU.getAccZ() - 1.0f;
    check[3] += myIMU.getGyroX();
    check[4] += myIMU.getGyroY();
    check[5] += myIMU.getGyroZ();
  }

  for (float& c : check)
    c /= cCheckSamples;

  const bool ok = fabsf(check[0]) < cMaxAccError  && fabsf(check[1]) < cMaxAccError  &&
                  fabsf(check[2]) < cMaxAccError  &&
                  fabsf(check[3]) < cMaxGyroError && fabsf(check[4]) < cMaxGyroError &&
                  fabsf(check[5]) < cMaxGyroError;

  Serial << "IMU calibration " << (ok ? "accepted" : "rejected") << ": deviation acc "
         << _FLOAT(check[0], 3) << ", " << _FLOAT(check[1], 3) << ", " << _FLOAT(check[2], 3)
         << " g, gyro " << _FLOAT(check[3], 2) << ", " << _FLOAT(check[4], 2) << ", "
         << _FLOAT(check[5], 2) << " deg/s" << endl;

  if (ok == false)
  {
    myIMU.setAccOffsets (oldAcc[0],  oldAcc[1],  oldAcc[2]);
    myIMU.setGyroOffsets(oldGyro[0], oldGyro[1], oldGyro[2]);
  }

  return ok;
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
    myUI.tft().drawString("Place the transmitter on a flat surface!", myUI.width()/2, 80, 4);
  }

  if (myPhase == EPhase::eCountdown)
  {
    // Remaining time is computed in unsigned arithmetic, the same idiom step() uses, so it stays
    // correct across the millis() rollover. Only the finished value becomes a float.
    const unsigned long elapsed   = millis() - myPhaseStartTime;
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
    // Padded to the countdown's width, so the countdown text is blanked as well.
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextPadding(myUI.tft().textWidth("Calibration starts in ...", 4));

    if (myPhase == EPhase::eMeasuring)
    {
      myUI.tft().setTextColor(TFT_WHITE, TFT_BLACK);
      myUI.tft().drawString("Calibrating ...", myUI.width()/2, 140, 4);
      myUI.tft().drawString(String(mySampleCount * 100 / cNumSamples) + " %", myUI.width()/2, 170, 4);
    }
    else if (myPhase == EPhase::eStored)
    {
      myUI.tft().setTextColor(TFT_GREEN, TFT_BLACK);
      myUI.tft().drawString("Calibration stored", myUI.width()/2, 140, 4);
      myUI.tft().fillRect(0, 170, myUI.width(), myUI.tft().fontHeight(4), TFT_BLACK); // the percentage
    }
    else
    {
      myUI.tft().setTextColor(TFT_RED, TFT_BLACK);
      myUI.tft().drawString("Calibration failed", myUI.width()/2, 140, 4);
      myUI.tft().setTextColor(TFT_WHITE, TFT_BLACK);
      myUI.tft().drawString("Keep it flat and still", myUI.width()/2, 170, 4);
    }

    myUI.tft().setTextPadding(0);
  }

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void IMUCalibration::start()
{
  myPhase = EPhase::eCountdown;
  myPhaseStartTime = millis();
  markRedraw();
  setState(EState::eActive);
}
