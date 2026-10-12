#pragma once

#include "MenuItem.h"
#include "InputController.h"

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
    InputController::Joystick& myJoyLeft;
    InputController::Joystick& myJoyRight;

    unsigned long myCalibrationStartTime = 0;
    EPhase myPhase = EPhase::eIdle;

    // Set by onClick() and consumed by step(). A request, not a phase - which is why it stays a
    // separate flag: the button can arrive at any point inside eMinMax or eResult.
    bool myStopRequested = false;

    joystickutilities::CenterSampler<3> myCenterLeft;         // Center samples left
    joystickutilities::CenterSampler<3> myCenterRight;        //   ... and right
    joystickutilities::RangeTracker<3>  myRangeLeft  {{}};    // Min/max travel left, seeded with the center
    joystickutilities::RangeTracker<3>  myRangeRight {{}};    //   ... and right

    unsigned long myResultStartTime = 0;                      // When eResult was entered
    joystickutilities::CalibrationResult<3> myResultLeft  {}; // Accepted axes left
    joystickutilities::CalibrationResult<3> myResultRight {}; //   ... and right
};