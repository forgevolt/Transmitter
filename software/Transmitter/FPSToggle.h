#pragma once

#include "MenuItem.h"

// ---- FPSToggle -------------------------------------------------------------------------
// Toggles the FPS display on or off. The setting is stored in NVS.

class FPSToggle : public MenuItem
{
  public:
    FPSToggle(UserInterface& ui, Menu& menu);
    virtual ~FPSToggle() {}

    // From class DisplayObject
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start() override;

    // true: the FPS are shown on screen
    bool isOn() const { return myShowFPS; }

  private:
    bool myShowFPS;
};
