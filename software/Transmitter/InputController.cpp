#include <Streaming.h>
#include "InputController.h"
#include "PinMap.h"

// ---- InputController -------------------------------------------------------------------

namespace
{

constexpr uint16_t cDebounceIntervalMs = 5; // 5 ms

} // namespace

// ----------------------------------------------------------------------------------------
InputController::InputController()
: myJoyL(cPinJoyLX, cPinJoyLY, cPinJoyLZ),
  myJoyR(cPinJoyRX, cPinJoyRY, cPinJoyRZ),
  mySwitches(cPinS1, cPinS2, cPinS3, cPinS4)
{
  // The mutex is created here rather than in begin() so that getState() is safe to call at any
  // time. It allocates from the FreeRTOS heap and touches no hardware, which is fine this early -
  // this object is a member of a global, so the constructor runs before setup().
  myMutex = xSemaphoreCreateMutex();
}

// ----------------------------------------------------------------------------------------
// Only reachable at program exit, which does not happen on this target: this object is a member of
// the global UserInterface. The guards matter anyway - vTaskDelete(nullptr) deletes the *calling*
// task, so an instance that never saw begin() would take the caller down with it.
InputController::~InputController()
{
  if (myTaskHandle != nullptr)
    vTaskDelete(myTaskHandle);

  if (myMutex != nullptr)
    vSemaphoreDelete(myMutex);
}

// ----------------------------------------------------------------------------------------
void InputController::begin()
{
  myBtnJoyL.attach(cPinJoyLBtn, INPUT_PULLUP);  
  myBtnJoyL.setPressedState(LOW);
  myBtnJoyL.interval(cDebounceIntervalMs);
  
  myBtnJoyR.attach(cPinJoyRBtn, INPUT_PULLUP);  
  myBtnJoyR.setPressedState(LOW);
  myBtnJoyR.interval(cDebounceIntervalMs);

  myJoyL.begin();
  myJoyR.begin();
 
  mySwitches.begin();

  if (myTaskHandle != nullptr) // begin() called twice
    return;

  // Created here, not in the constructor: every pin the task samples is configured by the calls
  // above, so the task only comes into existence once there is something valid to read. Pinning it
  // to core 1 (where loop() runs) avoids contending with WiFi/BT for CPU time if either is enabled.
  xTaskCreatePinnedToCore(processEvents, "processEvents",
                          2048,                 // Stack size in bytes
                          this,                 // Task input parameter
                          tskIDLE_PRIORITY + 1, // Priority of the task
                          &myTaskHandle,        // Task handle
                          1);                   // Core where the task should run
}

// ----------------------------------------------------------------------------------------
InputController::State InputController::getState()
{
  myJoyL.update();
  myJoyR.update();

  State ret;

  ret.up    = myJoyL.isUp()    || myJoyR.isUp();
  ret.down  = myJoyL.isDown()  || myJoyR.isDown();
  ret.left  = myJoyL.isLeft()  || myJoyR.isLeft();
  ret.right = myJoyL.isRight() || myJoyR.isRight();

  ret.LX = myJoyL.getX();  ret.LY = myJoyL.getY();  ret.LZ = myJoyL.getZ();
  ret.RX = myJoyR.getX();  ret.RY = myJoyR.getY();  ret.RZ = myJoyR.getZ();

  // The button and switch state is shared with the processEvents task, so it is read under the
  // lock. The joysticks are touched from loop() context alone - here and from the menu's
  // calibration screens.
  xSemaphoreTake(myMutex, portMAX_DELAY);

  ret.joyL          = myState.joyL;
  ret.joyR          = myState.joyR;
  ret.joyLisPressed = myState.joyLisPressed;
  ret.joyRisPressed = myState.joyRisPressed;

  ret.s1 = myState.s1;  ret.s2 = myState.s2;  ret.s3 = myState.s3;  ret.s4 = myState.s4;

  ret.s1Changed = myState.s1Changed;  ret.s2Changed = myState.s2Changed;
  ret.s3Changed = myState.s3Changed;  ret.s4Changed = myState.s4Changed;

  // The press and the switch change are edges, consumed by this read.
  myState.joyL = myState.joyR = false;
  myState.s1Changed = myState.s2Changed = myState.s3Changed = myState.s4Changed = false;

  xSemaphoreGive(myMutex);

  return ret;
}

// ----------------------------------------------------------------------------------------
void InputController::processEvents(void* pvParameters)
{
  InputController* c = static_cast<InputController*>(pvParameters);

  while (true) 
  {
    xSemaphoreTake(c->myMutex, portMAX_DELAY);
    
    // ---- Process buttons
    c->myBtnJoyL.update();
    if (c->myBtnJoyL.pressed())
    {
      c->myState.joyL = true;
    }
    c->myState.joyLisPressed = c->myBtnJoyL.isPressed();

    c->myBtnJoyR.update();
    if (c->myBtnJoyR.pressed())
    {
      c->myState.joyR = true;
    }
    c->myState.joyRisPressed = c->myBtnJoyR.isPressed();

    // ---- Process switches
    c->mySwitches.update();

    if (c->mySwitches.hasChangedS1()) { c->myState.s1Changed = true; }
    if (c->mySwitches.hasChangedS2()) { c->myState.s2Changed = true; }
    if (c->mySwitches.hasChangedS3()) { c->myState.s3Changed = true; }
    if (c->mySwitches.hasChangedS4()) { c->myState.s4Changed = true; }

    c->myState.s1 = c->mySwitches.isOnS1();
    c->myState.s2 = c->mySwitches.isOnS2();
    c->myState.s3 = c->mySwitches.isOnS3();
    c->myState.s4 = c->mySwitches.isOnS4();

    xSemaphoreGive(c->myMutex);

    // Every pass yields. The task sits just above tskIDLE_PRIORITY, where never blocking starves
    // the idle task on its core and trips the task watchdog. 2ms is well below the 5ms debounce
    // interval the buttons and switches use (see begin()), so responsiveness is unaffected.
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}

