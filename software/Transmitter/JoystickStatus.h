#pragma once

#include "DisplayObject.h"
#include "InputController.h"

// ---- JoystickStatus --------------------------------------------------------------------
// Displays the state of the three joystick axes on the screen.

class JoystickStatus : public DisplayObject
{
  public:
    JoystickStatus(UserInterface& ui, bool isLeft);
    ~JoystickStatus() override {};

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    void drawSliderH(float x, float y, int val);
    void drawSliderV(float x, float y, int val);
    void drawSliderA(float x, float y, int val, bool btnPressed);

  private:
    InputController::State myLastState;
    String myLastLabel;

    InputController::Joystick& myJoystick;
    bool myIsLeft;             // left or right joystick
};