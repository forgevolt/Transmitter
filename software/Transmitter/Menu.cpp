#include <Streaming.h>

#include "Menu.h"
#include "MenuItem.h"
#include "UserInterface.h"


// ---- Menu ------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
Menu::Menu(UserInterface& ui)
: DisplayObject(ui),
  myActiveItem(0)
{
  setState(EState::eNotActive);
}

// ----------------------------------------------------------------------------------------
Menu::~Menu()
{
  for (auto item : myMenuItems)
  {
    delete item;
  }
}

// ----------------------------------------------------------------------------------------
void Menu::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  // No registered MenuItems?
  else if (myMenuItems.empty() == true)
    return;

  // Processing of MenuItem is in progress
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->step(dt);

  else
  {
    if (myMenuItemStarted == true)
    {
      // The previously active MenuItem has finished; redraw the menu
      myMenuItemStarted = false;
      markRedraw();
    }

    for (auto item : myMenuItems)
    {
      item->step(dt);
    }
  }
}

// ----------------------------------------------------------------------------------------
void Menu::draw(bool forceRedraw)
{
  if (isActive() == false)
    return;

  // No registered MenuItems?
  if (myMenuItems.empty() == true)
    return;

  if (needsRedraw() == true)
    myUI.tft().fillScreen(TFT_BLACK);

  // Processing of MenuItem is in progress
  if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->draw(forceRedraw || needsRedraw());

  else
  {
    for (auto item : myMenuItems)
    {
      item->draw(forceRedraw || needsRedraw());
    }
  }

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void Menu::add(MenuItem* adoptMenuItem)
{
  if (adoptMenuItem == nullptr)
  {
    Serial << __PRETTY_FUNCTION__  << " -> adoptMenuItem == nullptr" << endl;
    return;
  }

  myMenuItems.push_back(adoptMenuItem);
}

// ----------------------------------------------------------------------------------------
void Menu::start()
{
  // y position of the first menu item
  int16_t y = 40;
  for (auto item : myMenuItems)
  {
    item->setPosY(y);
    item->setSelected(false);
    item->setState(EState::eNotActive);
    y += 45;
  }

  setActiveItem(0);
  myMenuItemStarted = false;

  setState(EState::eActive);
}

// ----------------------------------------------------------------------------------------
void Menu::stop()
{
  setState(EState::eNotActive);
}

// ----------------------------------------------------------------------------------------
void Menu::onClick()
{
  if (isActive() == false)
    return;

  // No registered MenuItems
  else if (myMenuItems.empty() == true)
    return;

  // Is a MenuItem active?
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->onClick();

  // Start the MenuItem
  else
  {
    myMenuItems[myActiveItem]->start();
    myMenuItemStarted = myMenuItems[myActiveItem]->isActive();
  }
}

// ----------------------------------------------------------------------------------------
void Menu::onUp()
{
  if (isActive() == false)
    return;

  // No registered MenuItems
  else if (myMenuItems.empty() == true)
    return;

  // Is a MenuItem active?
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->onUp();

  // Change active menu item
  else
    setActiveItem(myActiveItem-1);
}

// ----------------------------------------------------------------------------------------
void Menu::onDown()
{
  if (isActive() == false)
    return;

  // No registered MenuItems
  else if (myMenuItems.empty() == true)
    return;

  // Is a MenuItem active?
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->onDown();

  // Change active menu item
  else
    setActiveItem(myActiveItem+1);
}

// ----------------------------------------------------------------------------------------
void Menu::onRight()
{
  if (isActive() == false)
    return;

  // No registered MenuItems
  else if (myMenuItems.empty() == true)
    return;

  // Is a MenuItem active?
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->onRight();
}

// ----------------------------------------------------------------------------------------
void Menu::onLeft()
{
  if (isActive() == false)
    return;

  // No registered MenuItems
  else if (myMenuItems.empty() == true)
    return;

  // Is a MenuItem active?
  else if (myMenuItems[myActiveItem]->isActive() == true)
    myMenuItems[myActiveItem]->onLeft();
}

// ----------------------------------------------------------------------------------------
void Menu::setActiveItem(int num)
{
  if (myMenuItems.empty() == true)
    return;

  // Skip if the MenuItem is already active
  if (myMenuItems[myActiveItem]->isActive() == true)
    return;

  if (num < 0)
    num = myMenuItems.size()-1;
  else if (num >= int(myMenuItems.size()))
    num = 0;

  myMenuItems[myActiveItem]->setSelected(false);
  myMenuItems[num]->setSelected(true);

  myActiveItem = num;
}
