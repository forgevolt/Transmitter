#pragma once

#include <vector>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "InputController.h"
#include <SoundEngine.h>

// Constants related to the graphical objects

constexpr uint32_t cBorderCol   = 0xAD55;    // border color of the display objects
constexpr uint32_t cBorderColNA = 0x2945;    // border color when not active (e.g. joystick not installed)

// Colors shared by several display objects (RGB565)
constexpr uint32_t cBlueCol  = 0x4CBB;
constexpr uint32_t cGreenCol = 0x46AB;
constexpr uint32_t cRedCol   = 0xE0E3;

constexpr int32_t cTFTWidth         = 480;
constexpr int32_t cTFTHeight        = 320;

constexpr int32_t cStatusAreaWidth  = cTFTWidth - 20; // Status area: battery, receiver, FPS
constexpr int32_t cStatusAreaHeight = 40;
constexpr int32_t cStatusAreaPosX   = 10;
constexpr int32_t cStatusAreaPosY   = 10;
constexpr uint32_t cStatusAreaCol   = 0x10A2;

constexpr int32_t cMainAreaWidth  = cStatusAreaWidth;
constexpr int32_t cMainAreaHeight = 255;
constexpr int32_t cMainAreaPosX   = cStatusAreaPosX;
constexpr int32_t cMainAreaPosY   = 60;
constexpr uint32_t cMainAreaCol   = cStatusAreaCol;


// ---- UserInterface ---------------------------------------------------------------------
// Class UserInterface contains all the display and menu objects drawn on the screen,
// and implements the logic, acting as both the view and the controller.

class DisplayObject;
class Menu;

class UserInterface
{
  public:
    UserInterface();
    ~UserInterface();

    void begin();

    // Appends a display object to the main screen. The object is "adopted" and will be deleted by
    // this class's destructor.
    void addDisplayObject(DisplayObject* adoptObject);

    // Installs the menu, replacing and deleting any previous one. Separate from addDisplayObject()
    // rather than an overload of it: Menu derives from DisplayObject, so an overload pair would be
    // resolved on the static type at the call site, and a Menu* passed through a DisplayObject*
    // would be filed away as a main-screen object where it would never open. The object is
    // "adopted" and will be deleted by this class's destructor.
    void setMenu(Menu* adoptMenu);

    // Returns true if the menu is currently active
    bool isMenuActive();

    void startMenu();
    void stopMenu();

    void onClick();
    void onUp();
    void onDown();
    void onRight();
    void onLeft();

    // The main loop calls step() to compute the next state of the display objects.
    // The parameter 'dt' denotes the time elapsed since the last call to step().
    // A call to draw() renders all display objects.
    // loop() calls step(dt) and then draw() once per frame.
    void step(unsigned long dt);
    void draw();

    // Access TFT information
    TFT_eSPI& tft()  { return myTFT;          }
    int16_t height() { return myTFT.height(); }
    int16_t width()  { return myTFT.width();  }

    // Direct access to the physical buttons, switches and joysticks
    InputController& input()  { return myInput;  }
    const InputController::State& inputState() const { return myInputState; }

    // Sound-related methods
    SoundEngine& sound() { return mySound;  }

    // Sound methods to be called by MenuItem or DisplayObject instances
    void click();
    void signal();
    void beep();

  private:
    TFT_eSPI myTFT;
    InputController myInput;
    InputController::State myInputState;
    SoundEngine mySound;

    // Redraw flag handling
    bool myNeedsRedraw = true; // Dirty flag to trigger redraw
    bool needsRedraw() const { return myNeedsRedraw; }
    void markRedraw()        { myNeedsRedraw = true; }
    void clearRedrawFlag()   { myNeedsRedraw = false; }

    // Simple menu system
    Menu* myMenu;
    unsigned long myMenuStartDelay = 0; // Brief delay to let the switch turn green
                                        // on the display before the menu starts

    // Objects that are visualized on the display
    std::vector<DisplayObject*> myMainScreenObjects;
};