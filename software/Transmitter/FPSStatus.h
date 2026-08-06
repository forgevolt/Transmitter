#pragma once

#include "DisplayObject.h"

// ---- FPSStatus -------------------------------------------------------------------------
// Displays frame per seconds (FPS)

class FPSStatus : public DisplayObject
{
  public:
    FPSStatus(UserInterface& ui);
    ~FPSStatus() override {}

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    int myFPS;
    int myFPSCounter;
    int64_t myStime;
};