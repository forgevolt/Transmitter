#include <TFT_eSPI.h>
#include <Preferences.h>
#include "DeviceTypeConfiguration.h"
#include "images/img1Joy.h"
#include "images/img2Joy.h"

// ---- DeviceTypeConfiguration -----------------------------------------------------------

namespace
{

const String cMenuName               = "Device Type Configuration";
constexpr const char* cPreferencesNS = "Device";
constexpr const char* cKey           = "oneJoySetup";

const int32_t cLeftPosX     = 70;
const int32_t cPosY         = 140;
const uint32_t cSelectedCol = 0xE0E3;

} // namespace

// ----------------------------------------------------------------------------------------
DeviceTypeConfiguration::DeviceTypeConfiguration(UserInterface& ui, Menu& menu)
: MenuItem(ui, menu, cMenuName)
{}

// ----------------------------------------------------------------------------------------
void DeviceTypeConfiguration::draw(bool forceRedraw)
{
  if (isActive() == false)
  {
    MenuItem::draw(forceRedraw);
    return;
  }

  if (forceRedraw == false && needsRedraw() == false && needsPartialRedraw() == false)
    return;

  if (forceRedraw == true || needsRedraw() == true)  
  {
    myUI.tft().fillScreen(TFT_BLACK);
    myUI.tft().setTextDatum(TC_DATUM);
    myUI.tft().setTextColor(TFT_WHITE);
    myUI.tft().drawString(cMenuName, myUI.width()/2, 20, 4);
    myUI.tft().drawFastHLine(0, 60, myUI.width(), TFT_WHITE);
    myUI.tft().drawString("Please select the configuration", myUI.width()/2, 80, 4);
    myUI.tft().setTextDatum(MC_DATUM);
  }

  // 1-Joy - Button 
  myUI.tft().pushImage(cLeftPosX, cPosY, img1JoyWidth, img1JoyHeight, img1Joy);
  myUI.tft().drawSmoothRoundRect(cLeftPosX-4, cPosY-4, 9, 4, img1JoyWidth+8, img1JoyHeight+8, 
                                    (myIsOneJoySelected == true) ? cSelectedCol : TFT_BLACK, TFT_BLACK);

  // 2-Joy - Button
  myUI.tft().pushImage(myUI.width()-cLeftPosX-img2JoyWidth, cPosY, img2JoyWidth, img2JoyHeight, img2Joy);
  myUI.tft().drawSmoothRoundRect(myUI.width()-cLeftPosX-img2JoyWidth-4, cPosY-4, 9, 4, img2JoyWidth+8, img2JoyHeight+8, 
                                    (myIsOneJoySelected == false) ? cSelectedCol : TFT_BLACK, TFT_BLACK);

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void DeviceTypeConfiguration::start()
{
  // Read device configuration from preferences
  Preferences p;
  p.begin(cPreferencesNS, true); // read-only: does not create the namespace
  myIsOneJoySelected = p.getBool(cKey, true);
  p.end();

  markRedraw();
  setState(EState::eActive);
}

// ----------------------------------------------------------------------------------------
void DeviceTypeConfiguration::onClick()
{
  setState(EState::eNotActive);

  // Unchanged: just leave. A restart would drop the link to the receiver for nothing.
  if (myIsOneJoySelected == getSetup())
    return;

  // Set device configuration
  Preferences p;
  p.begin(cPreferencesNS);
  p.putBool(cKey, myIsOneJoySelected);
  p.end();

  // Restart, so that setup() applies the new configuration
  ESP.restart();
}

// ----------------------------------------------------------------------------------------
void DeviceTypeConfiguration::onRight()
{
  myIsOneJoySelected = !myIsOneJoySelected;
  markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
void DeviceTypeConfiguration::onLeft()
{
  myIsOneJoySelected = !myIsOneJoySelected;
  markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
bool DeviceTypeConfiguration::getSetup()
{
  // Read configuration from preferences
  Preferences p;
  p.begin(cPreferencesNS, true); // read-only: does not create the namespace
  bool oneJoySetup = p.getBool(cKey, true);
  p.end();

  return oneJoySetup;
}

