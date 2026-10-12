#pragma once

#include "MenuItem.h"
#include "InputController.h"

// ---- JoystickCalibration ---------------------------------------------------------------
// Calibration for one or two 3-axis joysticks: the center is averaged over releases from six
// directions, then the travel is swept.

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
      eCenter,         // joysticks are released from six directions; each rest position is measured
      eMinMax,         // user sweeps the full travel
      eResult          // lists the axes that were not accepted; only entered if there are any
    };

    // One joystick in eCenter: pushed in the direction of the current step, let go, holding
    // still, measured. The center is the average of its six rest positions.
    enum class ERelease { ePush, eLetGo, eSettle, eMeasure, eDone };

    struct CenterRelease
    {
      ERelease state = ERelease::ePush;
      unsigned long since = 0;                      // start of the still window or the measurement
      std::array<int, 3> lo {}, hi {};              // raw range within the still window
      joystickutilities::CenterSampler<3> sampler;  // the current release
      std::array<long, 3> sum {};                   // rest positions of the finished releases
      int count = 0;                                //   ... and how many there are
      int pushSign  = 0;                            // direction of the last push, +1 or -1
      int twistSign = 0;                            // direction of the first twist; the second goes the other way
    };

    void startReleaseStep();
    void stepRelease(InputController::Joystick& stick, CenterRelease& release, const char* nameSpace);
    bool isStill(const InputController::Joystick& stick, CenterRelease& release);
    static std::array<int, 3> releaseCenter(const CenterRelease& release);
    void startMinMax();
    void accumulateMinMax();
    void finish();

    void drawCenterScreen(bool forceRedraw);
    void drawReleaseStatus(const char* label, const CenterRelease& release, int y);
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

    int myReleaseStep = 0;                                    // Current step of eCenter, 0 to 5
    CenterRelease myReleaseLeft;                              // Center releases left
    CenterRelease myReleaseRight;                             //   ... and right
    joystickutilities::RangeTracker<3>  myRangeLeft  {{}};    // Min/max travel left, seeded with the center
    joystickutilities::RangeTracker<3>  myRangeRight {{}};    //   ... and right

    unsigned long myResultStartTime = 0;                      // When eResult was entered
    joystickutilities::CalibrationResult<3> myResultLeft  {}; // Accepted axes left
    joystickutilities::CalibrationResult<3> myResultRight {}; //   ... and right
};