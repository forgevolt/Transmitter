#include "Switches.h"

// ---- Switches --------------------------------------------------------------------------

namespace
{

constexpr uint16_t cDebounceIntervalMs = 5; // 5 ms

} // namespace

// ----------------------------------------------------------------------------------------
Switches::Switches(uint8_t pinS1, uint8_t pinS2, uint8_t pinS3, uint8_t pinS4) 
: myPinS1(pinS1), myPinS2(pinS2), myPinS3(pinS3), myPinS4(pinS4)
{}

// ----------------------------------------------------------------------------------------
void Switches::begin()
{
  myS1.attach(myPinS1, INPUT_PULLUP);  myS1.interval(cDebounceIntervalMs);
  myS2.attach(myPinS2, INPUT_PULLUP);  myS2.interval(cDebounceIntervalMs);
  myS3.attach(myPinS3, INPUT_PULLUP);  myS3.interval(cDebounceIntervalMs);
  myS4.attach(myPinS4, INPUT_PULLUP);  myS4.interval(cDebounceIntervalMs);
}

// ----------------------------------------------------------------------------------------
void Switches::update()
{
  myS1.update();
  myS2.update();
  myS3.update();
  myS4.update();
}

// ----------------------------------------------------------------------------------------
// Bounce::read() returns the debounced pin level, and a closed switch pulls the pin high.
bool Switches::isOnS1() const { return myS1.read(); }
bool Switches::isOnS2() const { return myS2.read(); }
bool Switches::isOnS3() const { return myS3.read(); }
bool Switches::isOnS4() const { return myS4.read(); }

// ----------------------------------------------------------------------------------------
bool Switches::hasChangedS1() const { return myS1.changed(); }
bool Switches::hasChangedS2() const { return myS2.changed(); }
bool Switches::hasChangedS3() const { return myS3.changed(); }
bool Switches::hasChangedS4() const { return myS4.changed(); }

