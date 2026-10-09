#pragma once

#include "DisplayObject.h"

class FPSToggle;

// ---- FPSStatus -------------------------------------------------------------------------
// Displays frame per seconds (FPS)

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
    int myFPS;
    int myFPSCounter;
    int64_t myStime;
};