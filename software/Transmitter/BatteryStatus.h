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

    // The gauge is read every 30 s - see step(). myCellPercent is -1 while there is no valid reading.
    unsigned long myLastUpdate;
    float myCellPercent;

    // Low battery: whether the warning is on, when the tone last played, and the blink phase.
    bool myIsLow = false;
    unsigned long myLowWarnedAt = 0;
    bool myBlinkOn = true;
};