#pragma once

#include "MenuItem.h"

// ---- FPSToggle -------------------------------------------------------------------------
// Allows to toggle (on/off) FPS information.

class FPSToggle : public MenuItem
{
  public:
    FPSToggle(UserInterface& ui, Menu& menu);
    virtual ~FPSToggle() {}

    // From DisplayObject
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start() override;

    // true: the FPS are shown on screen
    bool isOn() const { return myShowFPS; }

  private:
    bool myShowFPS;
};
