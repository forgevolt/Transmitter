#pragma once

#include "MPU6050_light.h"
#include "DisplayObject.h"

// ---- IMUStatus -------------------------------------------------------------------------
// Displays the state of the IMU on the screen incl. the temperature.

class IMUStatus : public DisplayObject
{
  public:
    IMUStatus(UserInterface& ui, MPU6050& imu);
    ~IMUStatus() override {};

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    MPU6050& myIMU;

    // "spot" position inside the IMU circle
    float mySX, mySY; 

    // Temperature is read every 15 s rather than on every draw(). The validity flag is kept
    // separate from the value because no reading can stand in for "not read yet" - every
    // temperature the sensor can report is a legitimate one, including the negative ones.
    unsigned long myLastTempUpdate;
    float myTemp;
    bool  myTempValid;
};