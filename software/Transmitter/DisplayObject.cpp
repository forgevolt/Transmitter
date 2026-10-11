#include "DisplayObject.h"
#include <Streaming.h>

// ---- DisplayObject ---------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
DisplayObject::DisplayObject(UserInterface& ui)
: myUI(ui),
  myState(EState::eActive),
  myBuf(TFT_eSprite(&ui.tft()))
{}

// ----------------------------------------------------------------------------------------
DisplayObject::~DisplayObject()
{}

// ----------------------------------------------------------------------------------------
DisplayObject::EState DisplayObject::getState() const
{
  return myState;
}

// ----------------------------------------------------------------------------------------
void DisplayObject::setState(EState state)
{
  // Ignore invalid state
  if (state == EState::eActive || state == EState::eNotActive)
    myState = state;
}

// ----------------------------------------------------------------------------------------
bool DisplayObject::isActive() const
{
  return (myState == EState::eActive);
}

// ----------------------------------------------------------------------------------------
bool DisplayObject::createBuffer(int width, int height, const char* site, bool& reported)
{
  if (myBuf.createSprite(width, height) != nullptr)
    return true;

  if (reported == false)
  {
    reported = true;

    // Free total next to largest single block is the informative pair: a large total with a
    // small largest block is fragmentation, which is what a per-frame allocation of this size
    // produces over hours of uptime.
    Serial << "ERROR: " << site << " -> no room for a " << width << "x" << height
           << " sprite (" << (width * height * 2) << " bytes). Free heap "
           << ESP.getFreeHeap() << ", largest block " << ESP.getMaxAllocHeap()
           << ". Drawing is skipped until an allocation succeeds." << endl;
  }

  return false;
}

// ----------------------------------------------------------------------------------------
void DisplayObject::drawButton(int x, int y, int length, const String& text, bool isActive)
{
  static bool reported = false;
  if (createBuffer(length, 34+2, __PRETTY_FUNCTION__, reported) == false)
    return;

  myBuf.fillSprite(TFT_BLACK);

  if (isActive == true)
    myBuf.fillSmoothRoundRect(0, 0, myBuf.width()-2, 34, 16, cBorderColNA, TFT_BLACK);
  myBuf.drawSmoothRoundRect(0, 0, 16, 15, myBuf.width()-2, 34, cBorderCol, cBorderColNA);

  myBuf.setTextColor(TFT_WHITE);
  myBuf.setTextDatum(MC_DATUM);
  myBuf.drawString(text, myBuf.width()/2, 19, 4);

  myBuf.pushSprite(x, y);
  myBuf.deleteSprite();
}
