#pragma once

#include "Adafruit_MAX1704X.h"
#include "DisplayObject.h"

// ---- BatteryStatus ---------------------------------------------------------------------
// Displays an icon with battery status on the screen.

class BatteryStatus : public DisplayObject
{
  public:
    BatteryStatus(UserInterface& ui, Adafruit_MAX17048& batteryMonitor);
    ~BatteryStatus() override {}

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    // Reference to the MAX17048 I2C battery monitor
    Adafruit_MAX17048& myFuelGauge;

    // Access the gauge every 30sec
    unsigned long myLastUpdate;
    float myCellPercent;
};