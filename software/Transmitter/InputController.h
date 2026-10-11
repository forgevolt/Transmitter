#pragma once

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <Bounce2.h>
#include "Joystick3Axis.h"
#include "Switches.h"


// ---- InputController -------------------------------------------------------------------
// Debounces the two joystick buttons and the four switches in a task on CPU core 1. The
// joysticks are sampled in getState(), from loop().


class InputController
{
  public:
    // Snapshot of every input.
    struct State
    {
      // true if the button was physically pressed (change from released to pressed)
      bool joyL = false, joyR = false;

      // true if the button is currently physically pressed
      bool joyLisPressed = false, joyRisPressed = false;

      // true == joystick moved in that direction
      bool up = false, down = false, left = false, right = false;

      int LX = 0, LY = 0, LZ = 0;   // joystick position (left)
      int RX = 0, RY = 0, RZ = 0;   // joystick position (right)

      // true if switch is on
      bool s1 = false, s2 = false, s3 = false, s4 = false;

      // true if that switch changed since the previous snapshot. Latched by the input task and
      // consumed by getState(), so a change is reported once however briefly it bounced.
      bool s1Changed = false, s2Changed = false, s3Changed = false, s4Changed = false;
    };

  public:
    InputController();
    ~InputController();

    // Initialize and start event processing
    void begin();

    // Returns a snapshot of every input. It samples the joysticks and consumes the button-press
    // and switch-change edges, so call it once per frame.
    State getState();

    // Direct access to the joystick objects, e.g. for calibration
    Joystick3Axis& joyL() { return myJoyL; }
    Joystick3Axis& joyR() { return myJoyR; }

    const String& getLabelBtnL() const { return myLabelBtnL; }
    void setLabelBtnL(const char* l)   { if (myLabelBtnL != l) myLabelBtnL = l; }
    const String& getLabelBtnR() const { return myLabelBtnR; }
    void setLabelBtnR(const char* l)   { if (myLabelBtnR != l) myLabelBtnR = l; }

    // Direct access to the Switches object, e.g. to set the labels
    Switches& switches() { return mySwitches; }

  private:
    // Joysticks with a single button each
    Bounce2::Button myBtnJoyL;
    Bounce2::Button myBtnJoyR;

    Joystick3Axis myJoyL;
    Joystick3Axis myJoyR;
    String myLabelBtnL, myLabelBtnR;

    Switches mySwitches;

    // Button and switch state as seen by the processEvents task
    State myState;

    static void processEvents(void* pvParameters);
    TaskHandle_t      myTaskHandle = nullptr;
    SemaphoreHandle_t myMutex      = nullptr;
};