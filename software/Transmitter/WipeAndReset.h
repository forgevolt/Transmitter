#pragma once

#include "MenuItem.h"

// ---- WipeAndReset ----------------------------------------------------------------------
// Erases the whole NVS partition - calibration data and every setting - and restarts.

class WipeAndReset : public MenuItem
{
  public:
    WipeAndReset(UserInterface& ui, Menu& menu);
    ~WipeAndReset() override {};

    // From class DisplayObject
    void draw(bool forceRedraw = false) override;

    // From class MenuItem
    void start()   override;
    void onClick() override;
    void onRight() override; 
    void onLeft()  override;

  private:
    bool myIsYesSelected = false;
};