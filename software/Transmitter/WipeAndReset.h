#pragma once

#include "MenuItem.h"

// ---- WipeAndReset ----------------------------------------------------------------------
// Clears all calibration data and other configuration stored in EEPROM.

class WipeAndReset : public MenuItem
{
  public:
    WipeAndReset(UserInterface& ui, Menu& menu);
    ~WipeAndReset() override {};

    // From class DisplayObject
    void draw(bool forceRedraw = false) override;

    // From MenuItem
    void start()   override;
    void onClick() override;
    void onRight() override; 
    void onLeft()  override;

  private:
    bool myIsYesSelected = false;
};