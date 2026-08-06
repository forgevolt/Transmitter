#pragma once

#include <Arduino.h>
#include <Bounce2.h>

// ---- Switches --------------------------------------------------------------------------
// Representation of the 4 switches of the transmitter.

class Switches
{
  public:
    Switches(uint8_t pinS1, uint8_t pinS2, uint8_t pinS3, uint8_t pinS4);

    // Initialize the IO pins.
    void begin();

    // Samples all four inputs. Called from the input task only.
    void update();

    // Debounced state as of the last update(). true == the switch is on.
    bool isOnS1() const;
    bool isOnS2() const;
    bool isOnS3() const;
    bool isOnS4() const;

    // true if that switch changed state at the last update().
    bool hasChangedS1() const;
    bool hasChangedS2() const;
    bool hasChangedS3() const;
    bool hasChangedS4() const;

    const String& getLabelS1() const { return myLabelS1; }
    void setLabelS1(const char* l)   { if (myLabelS1 != l) myLabelS1 = l; }
    const String& getLabelS2() const { return myLabelS2; }
    void setLabelS2(const char* l)   { if (myLabelS2 != l) myLabelS2 = l; }
    const String& getLabelS3() const { return myLabelS3; }
    void setLabelS3(const char* l)   { if (myLabelS3 != l) myLabelS3 = l; }
    const String& getLabelS4() const { return myLabelS4; }
    void setLabelS4(const char* l)   { if (myLabelS4 != l) myLabelS4 = l; }

  private:
    uint8_t myPinS1, myPinS2, myPinS3, myPinS4;
    Bounce myS1, myS2, myS3, myS4;
    String myLabelS1, myLabelS2, myLabelS3, myLabelS4;
};
