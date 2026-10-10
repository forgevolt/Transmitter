#include <nvs_flash.h>
#include <Streaming.h>
#include "WipeAndReset.h"


// ---- WipeAndReset ----------------------------------------------------------------------

namespace
{

const String cMenuName = "Wipe and Reset";

const int32_t cLeftPosX = 120;
const int32_t cPosY     = 193;
const int32_t cWidth    = 100;

} // namespace

// ----------------------------------------------------------------------------------------
WipeAndReset::WipeAndReset(UserInterface& ui, Menu& menu)
: MenuItem(ui, menu, cMenuName)
{}

// ----------------------------------------------------------------------------------------
void WipeAndReset::draw(bool forceRedraw)
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
    myUI.tft().drawString("Do you really want to reset to", myUI.width()/2, 80, 4);
    myUI.tft().drawString("default settings?", myUI.width()/2, 110, 4);
    myUI.tft().drawString("This erases calibration and settings.", myUI.width()/2, 140, 4);
    myUI.tft().setTextDatum(MC_DATUM);
  }

  // Yes - Button
  drawButton(cLeftPosX, cPosY, cWidth, "Yes", myIsYesSelected);

  // No - Button
  drawButton(myUI.width()-cLeftPosX-cWidth, cPosY, cWidth, "No", !myIsYesSelected);

  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void WipeAndReset::start()
{
  myIsYesSelected = false;
  markRedraw();
  setState(EState::eActive);
}

// ----------------------------------------------------------------------------------------
void WipeAndReset::onClick()
{
  if (myIsYesSelected == true)
  {
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init();  // initialize the NVS partition.

    // Restart, so that every setting returns to its default
    ESP.restart();
  }

  setState(EState::eNotActive);
}

// ----------------------------------------------------------------------------------------
void WipeAndReset::onRight()
{
  myIsYesSelected = !myIsYesSelected;
  markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
void WipeAndReset::onLeft()
{
  myIsYesSelected = !myIsYesSelected;
  markPartialRedraw();
}
