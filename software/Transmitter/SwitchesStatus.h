#pragma once

#include "DisplayObject.h"

// ---- SwitchesStatus --------------------------------------------------------------------
// Displays the state of the 4 switches on the screen.

class SwitchesStatus : public DisplayObject
{
  public:
    SwitchesStatus(UserInterface& ui);
    ~SwitchesStatus() override {}

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    void drawState(float x, float y, bool on, const String& label);

  private:
    InputController::State myLastState;
    String myLastLabelS1, myLastLabelS2, myLastLabelS3, myLastLabelS4;
};