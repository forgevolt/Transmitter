#pragma once

#include "MenuItem.h"

// ---- SoundVolume -----------------------------------------------------------------------
// Sets the master sound volume; stored in NVS as it changes

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