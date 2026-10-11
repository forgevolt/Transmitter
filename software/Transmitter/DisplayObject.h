#pragma once

#include "UserInterface.h"

// ---- DisplayObject ---------------------------------------------------------------------
// Abstract base class for objects to be displayed

class DisplayObject
{
  public:
    enum class EState
    {
      eActive,
      eNotActive
    };

  public:
    explicit DisplayObject(UserInterface& ui);
    virtual ~DisplayObject() = 0;

    // Update the state of the display object. 'dt' milliseconds have passed since the last call.
    virtual void step(unsigned long /* dt */) {}

    // Draw the object
    virtual void draw(bool forceRedraw = false) = 0;

    EState getState() const;
    void setState(EState state);

    // Returns true if display object is active and must be processed in loop()
    bool isActive() const;

    void drawButton(int x, int y, int length, const String& text, bool isActive);

    // Redraw flag handling
    bool needsRedraw() const        { return myNeedsRedraw; }
    bool needsPartialRedraw() const { return myNeedsPartialRedraw; }
    void markRedraw()               { myNeedsRedraw = true; }
    void markPartialRedraw()        { myNeedsPartialRedraw = true; }
    void clearRedrawFlags()         { myNeedsRedraw = myNeedsPartialRedraw = false; }

  protected:
    // Creates myBuf, and says so on the serial port when the heap cannot supply it. Returns
    // false when the sprite is unusable, and the caller must then not draw: TFT_eSPI discards
    // drawing to a sprite it never allocated, silently. 'reported' belongs to the caller so
    // each site speaks at most once - a failure recurs every frame, and thirty lines a second
    // would bury everything else on the port.
    bool createBuffer(int width, int height, const char* site, bool& reported);

    UserInterface& myUI;
    EState myState;

    TFT_eSprite myBuf;           // Off-screen buffer used to render graphics
    bool myNeedsRedraw = true;   // Dirty flags to trigger redraw
    bool myNeedsPartialRedraw = false;
};