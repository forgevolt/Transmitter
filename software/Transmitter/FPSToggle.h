#pragma once

#include "MenuItem.h"

// ---- FPSToggle -------------------------------------------------------------------------
// Allows to toggle (on/off) FPS information.

extern bool showFPS;

class FPSToggle : public MenuItem
{
  public:
    FPSToggle(UserInterface& ui, Menu& menu);
    virtual ~FPSToggle() {}

    // From DisplayObject
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start() override;
};
