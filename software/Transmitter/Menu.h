#pragma once

#include <vector>
#include "DisplayObject.h"


// ---- Menu ------------------------------------------------------------------------------

class MenuItem;

class Menu : public DisplayObject
{
  public:
    Menu(UserInterface& ui);
    ~Menu();

    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

    void add(MenuItem* adoptMenuItem);
    
    // Menu navigation
    void start();
    void stop();

    void onClick();
    void onUp(); 
    void onDown(); 
    void onRight(); 
    void onLeft(); 

  private:
    void setActiveItem(int num);

  private:
    // Objects that are visualized on the display 
    std::vector<MenuItem*> myMenuItems;

    int myActiveItem;
    bool myMenuItemStarted = false;
};
