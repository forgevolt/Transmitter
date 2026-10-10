#pragma once

#include "MPU6050_light.h"
#include "MenuItem.h"

// ---- IMUCalibration --------------------------------------------------------------------
// Calibrates the gyro and accelerometer offsets and stores them in NVS

class IMUCalibration : public MenuItem
{
  public:
    IMUCalibration(UserInterface& ui, Menu& menu, MPU6050& imu);
    ~IMUCalibration() override {};

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start() override;

  private:
    // Countdown before the measurement, the measurement itself (spread over several step()
    // calls), and the message shown with the result: stored or rejected.
    enum class EPhase { eCountdown, eMeasuring, eStored, eFailed };

    // Sets the measured offsets, then checks them against the readings they produce. Restores the
    // previous offsets and returns false if those are not the readings of a flat transmitter at rest.
    bool applyAndCheckOffsets();

    MPU6050& myIMU;
    EPhase myPhase = EPhase::eCountdown;
    unsigned long myPhaseStartTime = 0;

    // Measurement: samples taken so far and their sums (3 x acceleration, 3 x rotation rate).
    int   mySampleCount = 0;
    float mySum[6]      = {};
};