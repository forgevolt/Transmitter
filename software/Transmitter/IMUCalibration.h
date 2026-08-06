#pragma once

#include "MPU6050_light.h"
#include "MenuItem.h"

// ---- IMUCalibration --------------------------------------------------------------------
// Calculate gyro offsets

class IMUCalibration : public MenuItem
{
  public:
    IMUCalibration(UserInterface& ui, Menu& menu, MPU6050& imu);
    ~IMUCalibration() override {};

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

    // From MenuItem
    void start() override;

  private:
    MPU6050& myIMU;
    bool myIsCalibrationWaitPeriodActive = false;
    unsigned long myCalibrationStartTime = 0;
};