#include "UserInterface.h"
#include <Streaming.h>

#include "DisplayObject.h"
#include "Menu.h"
#include "PinMap.h"
#include "sounds/Startup.h"


// ---- UserInterface ---------------------------------------------------------------------

namespace
{

AudioClip startupClip(cSoundStartupWAV);

// Brief delay so draw() can turn the S1 indicator green before the menu paints over the screen.
constexpr unsigned long cMenuStartDelayMs = 5;

} // namespace

// ----------------------------------------------------------------------------------------
UserInterface::UserInterface()
: myTFT(),
  mySound(cI2S_LRC, cI2S_BCLK, cI2S_DOUT),
  myMenu(nullptr)
{}

// ----------------------------------------------------------------------------------------
UserInterface::~UserInterface()
{
  for (auto object : myMainScreenObjects)
  {
    delete object;
  }

  if (myMenu != nullptr)
    delete myMenu;
}

// ----------------------------------------------------------------------------------------
void UserInterface::begin()
{
  myTFT.init();
  myTFT.startWrite(); // TFT chip select held low permanently

  myTFT.setSwapBytes(true);
  myTFT.setRotation(1);
  myTFT.fillScreen(TFT_BLACK);

  // Display startup message
  myTFT.setTextDatum(MC_DATUM);
  myTFT.drawString("Starting up ...", myTFT.width()/2, 32, 4);

  myInput.begin();
  myInputState = myInput.getState();
  if (mySound.begin() == false)
    Serial << "ERROR: " << __PRETTY_FUNCTION__ << " -> sound engine init failed" << endl;    
  mySound.setVolume(mySound.click(), 200); // Otherwise, the switches will drown out the click sound
  mySound.setVolume(startupClip, 20);  
  
  mySound.play(startupClip);
}

// ----------------------------------------------------------------------------------------
void UserInterface::addDisplayObject(DisplayObject* adoptObject)
{
  if (adoptObject == nullptr)
  {
    Serial << __PRETTY_FUNCTION__  << " -> adoptObject == nullptr" << endl;
    return;
  }

  myMainScreenObjects.push_back(adoptObject);
}

// ----------------------------------------------------------------------------------------
void UserInterface::setMenu(Menu* adoptMenu)
{
  if (adoptMenu == nullptr)
  {
    Serial << __PRETTY_FUNCTION__  << " -> adoptMenu == nullptr" << endl;
    return;
  }

  if (myMenu != nullptr)
    delete myMenu;

  myMenu = adoptMenu;
}

// ----------------------------------------------------------------------------------------
bool UserInterface::isMenuActive()
{
  if (myMenu == nullptr)
    return false;

  else
    return myMenu->isActive();
}

// ----------------------------------------------------------------------------------------
void UserInterface::startMenu()
{
  // Check if we do have a menu
  if (myMenu == nullptr)
    return;

  else if (myMenu->isActive() == false) 
  {
    if (myMenuStartDelay == 0)
      myMenuStartDelay = millis();

    else if (millis() - myMenuStartDelay > cMenuStartDelayMs)
    {  
      myMenu->start();
      markRedraw(); 
    }
  }
}

// ----------------------------------------------------------------------------------------
void UserInterface::stopMenu()
{
  // Check if we do have a menu
  if (myMenu == nullptr)
    return;

  else if (myMenu->isActive() == true) 
  {
    myMenuStartDelay = 0;
    myMenu->stop();
    markRedraw();
  }
}

// ----------------------------------------------------------------------------------------
void UserInterface::onClick()
{
  // Check if we do have a menu
  if (myMenu == nullptr || myMenu->isActive() == false)
  {
    return;
  }
  else
  {
    myMenu->onClick();
  }
}

// ----------------------------------------------------------------------------------------
void UserInterface::onUp()
{
  // Hope we do have a menu and it is active
  if (myMenu == nullptr || myMenu->isActive() == false)
    return;

  myMenu->onUp();
}

// ----------------------------------------------------------------------------------------
void UserInterface::onDown()
{
  // Hope we do have a menu and it is active
  if (myMenu == nullptr || myMenu->isActive() == false)
    return;

  myMenu->onDown();
}

// ----------------------------------------------------------------------------------------
void UserInterface::onRight()
{
  // Hope we do have a menu and it is active
  if (myMenu == nullptr || myMenu->isActive() == false)
    return;

  myMenu->onRight();
}

// ----------------------------------------------------------------------------------------
void UserInterface::onLeft()
{
  // Hope we do have a menu and it is active
  if (myMenu == nullptr || myMenu->isActive() == false)
    return;

  myMenu->onLeft();
}

// ----------------------------------------------------------------------------------------
void UserInterface::step(unsigned long dt)
{
  // ---- Process input events

  myInputState = myInput.getState();

  // A switch that changed since the previous snapshot -> generate click sound
  if (myInputState.s1Changed == true) { click(); }
  if (myInputState.s2Changed == true) { click(); }
  if (myInputState.s3Changed == true) { click(); }
  if (myInputState.s4Changed == true) { click(); }

  (myInputState.s1 == true) ? startMenu() : stopMenu();

  if (myInputState.joyL == true || myInputState.joyR == true) { beep(); onClick(); }
  
  if (myInputState.up    == true) { onUp();    }
  if (myInputState.down  == true) { onDown();  }
  if (myInputState.right == true) { onRight(); }
  if (myInputState.left  == true) { onLeft();  }
      
  // ---- "step" menu or display objects

  if (myMenu != nullptr && myMenu->isActive() == true)
  {
    myMenu->step(dt);
  }
  else 
  {
    for (auto object : myMainScreenObjects)
    {
      if (object->isActive() == true)
        object->step(dt);
    } 
  }
}

// ----------------------------------------------------------------------------------------
void UserInterface::draw()
{
  if (needsRedraw() == true)
    myTFT.fillScreen(TFT_BLACK);

  if (myMenu != nullptr && myMenu->isActive() == true)
  {
    myMenu->draw(needsRedraw());
  }
  else 
  {
    // Two display areas (status & main)
    if (needsRedraw() == true)
    {
      myTFT.fillRoundRect(cStatusAreaPosX, cStatusAreaPosY, cStatusAreaWidth, cStatusAreaHeight, 5, cStatusAreaCol);
      myTFT.fillRoundRect(cMainAreaPosX, cMainAreaPosY, cMainAreaWidth, cMainAreaHeight, 5, cMainAreaCol);
    }

    for (auto object : myMainScreenObjects)
    {
      object->draw(needsRedraw());
    } 
  }

  clearRedrawFlag();
}

// ----------------------------------------------------------------------------------------
void UserInterface::click()
{
  mySound.play(mySound.click());
}

// ----------------------------------------------------------------------------------------
void UserInterface::signal()
{
  mySound.play(mySound.signal());
}

// ----------------------------------------------------------------------------------------
void UserInterface::beep()
{
  mySound.play(mySound.beep());
}
