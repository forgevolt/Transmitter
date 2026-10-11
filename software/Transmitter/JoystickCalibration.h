#pragma once

#include "MenuItem.h"
#include "Joystick3Axis.h"

// ---- JoystickCalibration ---------------------------------------------------------------
// Center and min/max calibration for one or two 3-axis joysticks.

class JoystickCalibration : public MenuItem
{
  public:
    JoystickCalibration(UserInterface& ui, Menu& menu);
    ~JoystickCalibration() override {}

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start()   override;
    void onClick() override;

  private:
    enum class EPhase
    {
      eIdle,           // not calibrating
      eCenterWait,     // joystick must be left alone; countdown on screen
      eCenterMeasure,  // averaging the center position
      eMinMax,         // user sweeps the full travel
      eResult          // lists the axes that were not accepted; only entered if there are any
    };

    void accumulateCenter();
    void startMinMax();
    void accumulateMinMax();
    void finish();

    void drawCenterScreen(bool forceRedraw);
    void drawMinMaxScreen(bool forceRedraw);
    void drawResultScreen(bool forceRedraw);

    // active says whether the axis exists; raw is only meaningful when it does.
    void drawSliderH(float x, float y, int min, int max, int center, uint16_t raw, bool active);
    void drawSliderV(float x, float y, int min, int max, int center, uint16_t raw, bool active);
    void drawSliderA(float x, float y, int min, int max, int center, uint16_t raw, bool active);

  private:
    Joystick3Axis& myJoyLeft;
    Joystick3Axis& myJoyRight;

    unsigned long myCalibrationStartTime = 0;
    EPhase myPhase = EPhase::eIdle;

    // Set by onClick() and consumed by step(). A request, not a phase - which is why it stays a
    // separate flag: the button can arrive at any point inside eMinMax or eResult.
    bool myStopRequested = false;

    int myCount = 0;                                    // Number of samples of the center calibration
    long mySumLX = 0, mySumLY = 0, mySumLZ = 0;         // Sum of all measurements for center calibration left
    long mySumRX = 0, mySumRY = 0, mySumRZ = 0;         //   ... and right
    int myCenterLX = 0, myCenterLY = 0, myCenterLZ = 0; // Center calibration values left
    int myCenterRX = 0, myCenterRY = 0, myCenterRZ = 0; //   ... and right
    int myMinLX = 0, myMinLY = 0, myMinLZ = 0;          // Min values measured left
    int myMinRX = 0, myMinRY = 0, myMinRZ = 0;          //   ... and right
    int myMaxLX = 0, myMaxLY = 0, myMaxLZ = 0;          // Max values measured left
    int myMaxRX = 0, myMaxRY = 0, myMaxRZ = 0;          //   ... and right

    unsigned long myResultStartTime = 0;                // When eResult was entered
    Joystick3Axis::CalibrationResult myResultLeft;      // Accepted axes left
    Joystick3Axis::CalibrationResult myResultRight;     //   ... and right
};