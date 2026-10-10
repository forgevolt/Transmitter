#pragma once

#include <Arduino.h>

// ---- Joystick3Axis ---------------------------------------------------------------------
// Representation of the 3-axis joystick. The joystick button is not processed within this 
// class.

class Joystick3Axis
{
  public: 
    // Min/max *raw* return values of getRawX/Y/Z methods
    static constexpr int cADC_MIN = 0;
    static constexpr int cADC_MAX = 4095; // ESP32, 12-bit
    
    // Min/max return values of the getX/Y/Z methods
    static constexpr int cMIN = -1000;
    static constexpr int cMAX =  1000;
    
  public:
    Joystick3Axis(int pinX, int pinY, int pinZ);

    // Enable / disable the joystick. This controls only whether the getters report a value;
    // the calibration data below is loaded and valid either way.
    void setActive(bool state) { myIsActive = state; }
    bool isActive() const      { return myIsActive;  }

    // Initialize and start processing
    void begin();

    // Calibration information that is taken into account when computing the current value 
    // of an axis (getter methods).
    void setCalibrationData(int minX, int maxX, int centerX,
                            int minY, int maxY, int centerY,
                            int minZ, int maxZ, int centerZ);

    // Samples all three axes and advances the four direction state machines from that sample.
    // Everything below reports it until the next call, so one update() per frame gives the
    // renderer and the transmit path the same numbers.
    void update();

    // The value from the last update(). Range is [-1000, 1000], 0 at center - also what an
    // inactive joystick reports.
    int getX() const { return myValueX; }
    int getY() const { return myValueY; }
    int getZ() const { return myValueZ; }
 
    // Raw values, e.g. for calibration. An inactive joystick reads 0 without
    // touching the ADC.
    uint16_t getRawX() { return readRaw(myPinX); }
    uint16_t getRawY() { return readRaw(myPinY); }
    uint16_t getRawZ() { return readRaw(myPinZ); }

    // Direction "keys" as of the last update(): true on the edge into a direction and again on
    // each auto-repeat. All four advance on every update(), so holding one does not leave the
    // others' timers stale.
    bool isUp()    const { return myIsUp;    }
    bool isDown()  const { return myIsDown;  }
    bool isRight() const { return myIsRight; }
    bool isLeft()  const { return myIsLeft;  }

  private:
    bool myIsActive = true; // true: joystick delivers value, i.e. it is installed

    int myPinX, myPinY, myPinZ;

    // Calibration, overwritten by begin() from Preferences and by setCalibrationData(). The
    // defaults describe an uncalibrated stick spanning the full ADC range, which keeps the
    // getters inside [cMIN, cMAX] and monotonic before any calibration has been read.
    int myMinX = cADC_MIN, myMaxX = cADC_MAX, myCenterX = cADC_MAX/2;
    int myMinY = cADC_MIN, myMaxY = cADC_MAX, myCenterY = cADC_MAX/2;
    int myMinZ = cADC_MIN, myMaxZ = cADC_MAX, myCenterZ = cADC_MAX/2;

    // Values and direction flags from the last update().
    int  myValueX = 0, myValueY = 0, myValueZ = 0;
    bool myIsUp = false, myIsDown = false, myIsRight = false, myIsLeft = false;

    // Reads one axis and maps it to [cMIN, cMAX]. The three axes differ only in their pin and
    // their calibration, so they share this rather than three copies of the same arithmetic.
    int readAxis(int pin, int min, int max, int center) const;

    // The unmapped counterpart, shared by getRawX/Y/Z for the same reason.
    uint16_t readRaw(int pin) const;

    // Per-direction state for the on/off-threshold + auto-repeat state machine shared by
    // isUp()/isDown()/isLeft()/isRight() - see updateDirectionState().
    struct DirectionState
    {
      bool myIsActive = false;
      bool myIsRepeatActive = false;
      unsigned long myTimeActive = 0;
      unsigned long myTimeRepeatTriggered = 0;
    };

    // Stores one axis's calibration if it describes usable travel, and falls back to the
    // full-range defaults if it does not. Applied to data from Preferences and from
    // setCalibrationData() alike, so no route into the members bypasses the check.
    void setAxisCalibration(int& min, int& max, int& center,
                            int newMin, int newMax, int newCenter, const char* axis);

    // Shared on/off-threshold + auto-repeat state machine. 'value' must already be signed so
    // that a positive value means "this direction is currently active" - e.g. isLeft() and
    // isDown() pass in the negated axis value.
    bool updateDirectionState(int value, DirectionState& state);

    DirectionState myUpState, myDownState, myLeftState, myRightState;
};
