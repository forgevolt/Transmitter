#pragma once

#include "DisplayObject.h"

class FPSToggle;

// ---- FPSStatus -------------------------------------------------------------------------
// Displays the frame rate (frames per second)

class FPSStatus : public DisplayObject
{
  public:
    FPSStatus(UserInterface& ui, const FPSToggle& toggle);
    ~FPSStatus() override {}

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    const FPSToggle& myToggle;
    int myFPS;               // -1: no complete count yet
    int myFPSCounter;
    int64_t myStime;
    int64_t myLastStep;      // to notice the pause while the menu is open - see step()
};