#include "SoundVolume.h"
#include <Streaming.h>
#include <Preferences.h>


// ---- SoundVolume -----------------------------------------------------------------------

namespace
{

const String cMenuName = "Sound Volume";

const float cRectWidth = 15;    // H/V slider width
const float cRectR     = 8;     // H/V rounded rectangle radius
const float cMaxVolume = 200;   // 200%
const float cVolumeInc = 10;    // 10% increments

constexpr const char* cPreferencesNS = "Sound";
constexpr const char* cKey           = "volume";

} // namespace

// ----------------------------------------------------------------------------------------
SoundVolume::SoundVolume(UserInterface& ui, Menu& menu)
: MenuItem(ui, menu, cMenuName)
{
  // Read volume setting from EEPROM or use default values
  Preferences p;
  p.begin(cPreferencesNS);
  myUI.sound().setVolume(p.getInt(cKey, 100));   // 100 = original/unmodified volume
  p.end();
}

// ----------------------------------------------------------------------------------------
void SoundVolume::draw(bool forceRedraw)
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
    myUI.tft().drawString("Move joystick left/right to change volume.", myUI.width()/2, 80, 4);
    myUI.tft().drawString("Press button to store and exit.", myUI.width()/2, 120, 4);
  }

  // ---- Partial redraw

  // Visualize sound volume with a "progress bar" 
  myUI.tft().drawSmoothRoundRect(100-cRectR, 170-cRectWidth/2, cRectR, cRectR-1, 
                                 myUI.width()-200+2*cRectR, cRectWidth, cBorderCol, TFT_BLACK);
  myUI.tft().drawWideLine(100, 170, 100+myUI.width()-200, 170, cRectWidth-5, TFT_BLACK, TFT_BLACK);
  myUI.tft().drawWideLine(100, 170, 100+float(myUI.width()-200)/cMaxVolume*myUI.sound().getVolume(), 170, cRectWidth-5, TFT_BLUE, TFT_BLUE);
  myUI.tft().setTextDatum(MC_DATUM);
  myUI.tft().setTextColor(TFT_WHITE);
  myUI.tft().drawNumber(myUI.sound().getVolume(), myUI.width()/2, 170, 2);
  
  clearRedrawFlags();
}

// ----------------------------------------------------------------------------------------
void SoundVolume::start()
{
  markRedraw();
  setState(EState::eActive);
}

// ----------------------------------------------------------------------------------------
void SoundVolume::onClick()
{
  int volume = myUI.sound().getVolume();
  
  // Make volume setting persistent
  Preferences p;
  p.begin(cPreferencesNS);
  if (volume != p.getInt(cKey, 100))
    p.putInt(cKey, volume);
  p.end();

  setState(EState::eNotActive);
}

// ----------------------------------------------------------------------------------------
void SoundVolume::onRight()
{
  myUI.sound().setVolume(myUI.sound().getVolume() + cVolumeInc);
  myUI.beep();
  markPartialRedraw();
}

// ----------------------------------------------------------------------------------------
void SoundVolume::onLeft()
{
  myUI.sound().setVolume(myUI.sound().getVolume() - cVolumeInc);
  myUI.beep();
  markPartialRedraw();
}
