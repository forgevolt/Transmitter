#include <TFT_eSPI.h>
#include <Streaming.h>
#include "MenuItem.h"

// ---- MenuItem --------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
MenuItem::MenuItem(UserInterface& ui, Menu& menu, const String& name)
: DisplayObject(ui),
  myMenu(menu),
  myMenuName(name)
{}

// ----------------------------------------------------------------------------------------
void MenuItem::draw(bool forceRedraw)
{
  if (needsRedraw() == false && forceRedraw == false)
    return;

  clearRedrawFlags();

  drawButton(70, myPosY-17, myUI.width()-2*70, myMenuName, myIsSelected);
}

// ----------------------------------------------------------------------------------------
void MenuItem::setSelected(bool selected) 
{ 
  myIsSelected = selected; 
  markRedraw();
}
