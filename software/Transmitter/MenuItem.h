#pragma once

#include "Menu.h"

// ---- MenuItem --------------------------------------------------------------------------
// Abstract base class for menu entries.

class Menu;

class MenuItem : public DisplayObject
{
  public:
    MenuItem(UserInterface& ui, Menu& menu, const String& name);
    virtual ~MenuItem() {}

    // From DisplayObject
    void step(unsigned long /* dt */) override {}
    void draw(bool forceRedraw = false) override;

    // Start processing the menu item function
    virtual void start() = 0;

    void setPosY(int16_t y)         { myPosY = y; }
    void setSelected(bool selected);
    
    // Process buttons and joystick movements
    virtual void onClick()       {}
    virtual void onUp()          {}
    virtual void onDown()        {} 
    virtual void onRight()       {} 
    virtual void onLeft()        {}

    const String& getName() const { return myMenuName; }

  protected:
    Menu& myMenu; // Reference to the menu e.g. to call stop() in order to exit
    String myMenuName;
    int16_t myPosY = 0;
    bool myIsSelected = false;
};

