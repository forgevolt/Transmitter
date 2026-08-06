#pragma once

#include "MenuItem.h"

// ---- SoundVolume -----------------------------------------------------------------------
// Set global sound volume

class SoundVolume : public MenuItem
{
  public:
    SoundVolume(UserInterface& ui, Menu& menu);
    ~SoundVolume() override {};

    // From class DisplayObject
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start() override;
    void onClick() override;
    void onRight() override; 
    void onLeft()  override;
};