#pragma once

#include "MenuItem.h"


// ---- DeviceTypeConfiguration -----------------------------------------------------------
// Configure the type of transmitter (1- or 2-joystick setup)

class DeviceTypeConfiguration : public MenuItem
{
  public:
    DeviceTypeConfiguration(UserInterface& ui, Menu& menu);
    virtual ~DeviceTypeConfiguration() {}

    // From class DisplayObject
    void draw(bool forceRedraw = false) override;

    // From MenuItem
    void start()   override;
    void onClick() override;
    void onRight() override; 
    void onLeft()  override;

    // true:  1-joystick setup configured
    // false: 2-joystick setup
    static bool getSetup();

  private:
    bool myIsOneJoySelected = false;
};
