// ---------------------------------------------------------------------------------------------
// Transmitter - controls a receiver (e.g. a hexapod). 
//               Supports up to two 3-axis joysticks, 4 switches, 2 buttons, and an IMU.
//               Includes a 480x320 display for a simple menu system and for visualizing  
//               input devices (e.g., joysticks) and the active receiver.
//           
//           Christoph Streit - 2024/2025
// ---------------------------------------------------------------------------------------------

#include <Streaming.h>
#include <WiFi.h>

// ---- Transmitter Configuration

static bool hasOneJoystick = true;

// ---- Fuel Gauge

#include "Adafruit_MAX1704X.h"

Adafruit_MAX17048 fuelGauge;

// ---- User Interface

#include "UserInterface.h"
#include "Menu.h"
#include "MenuItem.h"
#include "BatteryStatus.h"
#include "ReceiverStatus.h"
#include "FPSToggle.h"
#include "FPSStatus.h"
#include "WipeAndReset.h"
#include "DeviceTypeConfiguration.h"
#include "SoundVolume.h"

UserInterface ui;

// ---- Joystick(s)

#include "JoystickStatus.h"
#include "JoystickCalibration.h"

// ---- Switches

#include "SwitchesStatus.h"

// ---- IMU

#include <Wire.h>
#include "MPU6050_light.h"
#include "IMUStatus.h"
#include "IMUCalibration.h"

MPU6050 imu(Wire);

// ---- Transmitter (using ESP NOW)

#include "Transmitter.h"

Transmitter* transmitter = nullptr;

// How often a TransmitterData frame goes out. ESPNowUtilities drops a peer after cPeerTimeoutMs
// (1000ms) of silence *from that peer* (ESPNowConnection::removeLostPeer), so this interval is what
// keeps the robot's view of this transmitter alive. What keeps our view of the robot alive is the
// robot's own telemetry cadence, not this value.
constexpr unsigned long cSendIntervalMs = 50;

// Animation steps: a maximum of "targetFPS" is rendered. frameTimeMs is the budget one frame is
// given - loop() sleeps out whatever is left of it.
constexpr unsigned long targetFPS   = 30;
constexpr unsigned long frameTimeMs = 1000 / targetFPS;

// ---- Misc

#include "PinMap.h"

// Result of each hardware init. Serial is not visible in the field, and the TFT is not alive until
// ui.begin() - which runs after all three - so the results are collected here and shown once there
// is something to show them on.
struct StartupStatus
{
  bool fuelGauge = true;
  bool imu       = true;
  bool radio     = true;

  bool allOk() const { return fuelGauge == true && imu == true && radio == true; }
};

static StartupStatus startupStatus;

//----------------------------------------------------------------------------------------------
// Lists whatever failed and holds long enough to be read. Called after ui.begin(), so the TFT is
// initialised; the main screen paints over this on the first pass through loop().
static void showStartupFailures()
{
  ui.tft().fillScreen(TFT_BLACK);
  ui.tft().setTextDatum(TC_DATUM);
  ui.tft().setTextColor(TFT_RED, TFT_BLACK);
  ui.tft().drawString("Hardware not found", ui.width()/2, 40, 4);

  ui.tft().setTextColor(TFT_WHITE, TFT_BLACK);
  int y = 100;

  if (startupStatus.fuelGauge == false)
  {
    ui.tft().drawString("Battery gauge (MAX17048)", ui.width()/2, y, 2);
    y += 30;
  }
  if (startupStatus.imu == false)
  {
    ui.tft().drawString("IMU (MPU-6050)", ui.width()/2, y, 2);
    y += 30;
  }
  if (startupStatus.radio == false)
  {
    ui.tft().drawString("ESP-NOW radio", ui.width()/2, y, 2);
    y += 30;
  }

  ui.tft().drawString("Continuing without it", ui.width()/2, y + 20, 2);

  delay(10000);
}

//----------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  // Give the USB-to-UART bridge and host OS time to stabilize and open the port
  delay(700);

  Serial << "SW version from " << __DATE__ << " " << __TIME__ << endl;
  
  esp_log_level_set("*", ESP_LOG_VERBOSE); // Set Core Debug Level in Arduine IDE (Tools). Options are None to Verbose

  // The ADC settings are pinned rather than inherited: Joystick3Axis::cADC_MAX assumes 12-bit
  // conversions, and the joystick travel assumes the ~3.3V span that 11dB attenuation gives.
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Always set the pins explicitly.
  // 400kHz rather than the 100kHz Wire.begin() substitutes when given no frequency. The MPU-6050
  // read is 153 bit-times: 1.53ms at 100kHz against 0.38ms here, in a 33ms frame at 30fps.
  Wire.begin(cI2C_SDA, cI2C_SCL, 400000);
  SPI.begin(cSPI_SCK, cSPI_MISO, cSPI_MOSI, cSPI_SS_CS);
  
  // ---- Configuration

  hasOneJoystick = DeviceTypeConfiguration::getSetup();

  // ---- Fuel Gauge 

  startupStatus.fuelGauge = fuelGauge.begin();
  if (startupStatus.fuelGauge == false)
  {
    Serial << "Couldn't find MAX17048\nMake sure a battery is plugged in!" << endl;
  }

  // fuelGauge.quickStart();

  // ---- Joysticks

  if (hasOneJoystick == true)
  {
    Serial << "1-joystick configuration" << endl;
    ui.input().joyR().setActive(false);
  }
  else
  {
    Serial << "2-joystick configuration" << endl;
  }
 
  // ---- IMU

  // begin() returns the status of its first register write, so 0 means the device ACKed. The
  // library does not check its remaining writes, which makes this a presence test rather than a
  // health check - but it is enough to keep loop() off a bus with nothing on it.
  startupStatus.imu = (imu.begin() == 0);
  if (startupStatus.imu == false)
  {
    Serial << "ERROR: " << __PRETTY_FUNCTION__ << " -> MPU-6050 did not respond" << endl;
  }

  // ---- ESP-NOW 

  transmitter = new Transmitter((hasOneJoystick == true) ? eTransmitter1Joy : eTransmitter2Joy);
  startupStatus.radio = transmitter->begin(ESPNowConnection::cDefaultWifiChannel);
  if (startupStatus.radio == false)
  {
    Serial << "ERROR: " << __PRETTY_FUNCTION__ << " -> transmitter->begin failed" << endl;
  }

  // ---- Display and menu objects

  ui.addDisplayObject(new BatteryStatus(ui, fuelGauge));
  ui.addDisplayObject(new ReceiverStatus(ui, *transmitter));
  ui.addDisplayObject(new SwitchesStatus(ui));
  if (startupStatus.imu == true)
    ui.addDisplayObject(new IMUStatus(ui, imu));
  ui.addDisplayObject(new JoystickStatus(ui, true));
  ui.addDisplayObject(new JoystickStatus(ui, false));
  ui.addDisplayObject(new FPSStatus(ui));

  Menu* m = new Menu(ui);
  m->add(new WipeAndReset(ui, *m));
  m->add(new JoystickCalibration(ui, *m));
  if (startupStatus.imu == true)
    m->add(new IMUCalibration(ui, *m, imu));
  m->add(new DeviceTypeConfiguration(ui, *m));
  m->add(new SoundVolume(ui, *m));
  m->add(new FPSToggle(ui, *m));
  ui.setMenu(m);

  ui.begin();


  if (startupStatus.allOk() == false)
    showStartupFailures();

  // The budget the render loop draws against. The two figures matter separately: the total is
  // what is left after WiFi, TFT_eSPI, the audio buffers and every display object, while the
  // largest block is what a single createSprite() can actually get. The joystick sprite alone
  // asks for 62 KB of that in one piece, on every frame it redraws.
  Serial << "heap after setup: " << ESP.getFreeHeap() << " free, "
         << ESP.getMaxAllocHeap() << " largest block" << endl;
}

//----------------------------------------------------------------------------------------------
void loop()
{
  // Values that must outlive one frame. previousFrameStart is what makes dt a measurement, and
  // btnLeft/btnRight latch a press until it is sent - as plain locals they would be cleared every
  // frame and every press would be lost.
  static int64_t previousFrameStart = esp_timer_get_time() - frameTimeMs * 1000;
  static bool btnLeft  = false;
  static bool btnRight = false;

  const int64_t t0 = esp_timer_get_time();

  // Measured, not nominal - which is what step() is documented to receive. A frame that overruns
  // its budget reports the time it actually took, so anything that integrates dt stays in real
  // time instead of quietly running slow.
  const unsigned long dt = (t0 - previousFrameStart) / 1000;
  previousFrameStart = t0;

  // ---- IMU
  // Fast updates (no delays!)

  // Skipped when the IMU did not answer at startup: without this the loop drives failing I2C
  // transactions at 30Hz forever. IMUStatus and IMUCalibration are not created in that case
  // either, so nothing downstream reads a sensor that is not there.
  if (startupStatus.imu == true)
    imu.update();

  // ---- Update ui objects and render

  Transmitter::PeerData p = transmitter->getPeerData();

  // switchLabels[0] belongs to SWITCH 1 and is ignored while that switch opens the menu.
  // Freeing SWITCH 1 later means replacing the line below with p.switchLabels[0].
  ui.input().switches().setLabelS1("Menu");
  ui.input().switches().setLabelS2(p.switchLabels[1]);
  ui.input().switches().setLabelS3(p.switchLabels[2]);
  ui.input().switches().setLabelS4(p.switchLabels[3]);
  ui.input().setLabelBtnL(p.btnLabels[0]);
  ui.input().setLabelBtnR(p.btnLabels[1]);

  ui.step(dt);
  ui.draw();

  // If any button is pressed, store it for later reporting to the receiver
  if (ui.inputState().joyL == true) btnLeft  = true;
  if (ui.inputState().joyR == true) btnRight = true;

  // Forget button presses if menu is active or no receiver is present
  if (ui.isMenuActive() == true || transmitter->isPaired() == false)
  {
    btnLeft = btnRight = false;
  }

  // ---- Handle receiver(s)

  // Remove the receiver if it has not sent a heartbeat for a while => communication is lost
  transmitter->removeLostPeer();

  // ---- Send data to receivers

  if (transmitter->isPaired() == true)
  {
    // Send message every 50ms
    static unsigned long lastMsgUpdate = 0;
    static TransmitterData td;

    if (millis() - lastMsgUpdate > cSendIntervalMs)
    {
      lastMsgUpdate = millis();

      if (ui.isMenuActive())
      {
        // Menu open: keep the link alive, but command nothing. Going silent instead would be
        // indistinguishable from a dead transmitter at the receiving end.
        td.joyLX = td.joyLY = td.joyLZ = 0;
        td.joyRX = td.joyRY = td.joyRZ = 0;

        td.joyLBtn = td.joyRBtn = false;
        btnLeft = btnRight = false;

        td.switch1 = ui.inputState().s1;
        td.switch2 = ui.inputState().s2;
        td.switch3 = ui.inputState().s3;
        td.switch4 = ui.inputState().s4;
  
        td.angleX = td.angleY = 0.0f; 
      }
      else
      {
        td.joyLX = ui.inputState().LX;
        td.joyLY = ui.inputState().LY;
        td.joyLZ = ui.inputState().LZ;
        
        td.joyRX = ui.inputState().RX;
        td.joyRY = ui.inputState().RY;
        td.joyRZ = ui.inputState().RZ;

        td.joyLBtn = btnLeft;
        td.joyRBtn = btnRight;
        btnLeft = btnRight = false;

        td.switch1 = ui.inputState().s1;
        td.switch2 = ui.inputState().s2;
        td.switch3 = ui.inputState().s3;
        td.switch4 = ui.inputState().s4;
  
        // Sensor Orientation: X-axis points right, Y-axis points toward the top of the transmitter.  
        td.angleX = imu.getAngleY(); 
        td.angleY = imu.getAngleX();          
      }

      transmitter->send(td);
    }
  }

  // If input processing, message transmission to receiver, computation, and rendering 
  // complete faster than the target frame time (max frames per second), wait the remaining time.    
  const int64_t t1      = esp_timer_get_time();  // us
  const int     delayMS = (frameTimeMs - (t1 - t0) / 1000);
  if (delayMS > 0)
    vTaskDelay(delayMS / portTICK_PERIOD_MS);
}

