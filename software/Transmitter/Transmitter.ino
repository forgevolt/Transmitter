// ---------------------------------------------------------------------------------------------
// Transmitter - controls a receiver (e.g. a hexapod).
//               Supports up to two 3-axis joysticks, 4 switches, 2 buttons, and an IMU.
//               Includes a 480x320 display for a simple menu system and for visualizing
//               input devices (e.g. joysticks) and the active receiver.
//
//           Christoph Streit - 2024-2026
// ---------------------------------------------------------------------------------------------

#include <Streaming.h>
#include <WiFi.h>
#include <esp_system.h>    // esp_reset_reason()
#include <esp_core_dump.h> // esp_core_dump_get_summary()
#include <iterator>        // std::size

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

// ---- Transmitter (ESP-NOW)

#include "Transmitter.h"

Transmitter* transmitter = nullptr;

// loop() runs two jobs, each on its own schedule, and sleeps until the next one is due:
//
// - Screen: input, step() and draw(), at most "targetFPS" times a second.
// - Send: one TransmitterData frame every cSendIntervalMs (20 Hz). These frames also keep the link
//   alive: the receiver drops this transmitter after cPeerTimeoutMs (1000 ms) without one.
//
// Changing one rate does not change the other.
constexpr unsigned long targetFPS       = 30;
constexpr unsigned long frameTimeMs     = 1000 / targetFPS;
constexpr unsigned long cSendIntervalMs = 50;

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
// initialized; the main screen paints over this on the first pass through loop().
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

  ui.tft().drawString("Continuing anyway", ui.width()/2, y + 20, 2);

  delay(10000);
}

//----------------------------------------------------------------------------------------------
static const char* resetReasonName(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "external reset pin";
    case ESP_RST_SW:        return "software restart";
    case ESP_RST_PANIC:     return "panic / exception";
    case ESP_RST_INT_WDT:   return "interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "task watchdog";
    case ESP_RST_WDT:       return "other watchdog";
    case ESP_RST_DEEPSLEEP: return "deep-sleep wake";
    case ESP_RST_BROWNOUT:  return "brownout - supply voltage dipped";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "unknown";
  }
}

//----------------------------------------------------------------------------------------------
// Prints the core dump a panic left in flash (task, PC, backtrace), then erases it. Decode the
// addresses with the .elf of the same build (ESP Exception Decoder or xtensa-esp32-elf-addr2line).
// The UART cannot tell whether a monitor is open, so a boot with nobody watching loses the dump.
static void reportCoreDump()
{
  if (esp_core_dump_image_check() != ESP_OK)
    return; // no dump stored, or a corrupted one

  esp_core_dump_summary_t summary;
  if (esp_core_dump_get_summary(&summary) != ESP_OK)
    return;

  Serial << "Core dump from an earlier crash: task " << summary.exc_task
         << ", PC 0x" << _HEX(summary.exc_pc) << endl;

  Serial << "  Backtrace:";
  for (uint32_t i = 0; i < summary.exc_bt_info.depth && i < std::size(summary.exc_bt_info.bt); i++)
    Serial << " 0x" << _HEX(summary.exc_bt_info.bt[i]);
  Serial << (summary.exc_bt_info.corrupted ? " (corrupted)" : "") << endl;

  esp_core_dump_image_erase();
}

//----------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  // Give the USB-to-UART bridge and host OS time to stabilize and open the port
  delay(700);

  Serial << "SW version from " << __DATE__ << " " << __TIME__
         << ", app protocol version " << int(cAppProtocolVersion) << endl;

  // Why did the last run end? Anything but a power-on or a deliberate reset is a panic, a watchdog
  // or a brownout (battery empty), which would otherwise look like a normal start.
  const esp_reset_reason_t resetReason = esp_reset_reason();
  const bool abnormalReset = (resetReason != ESP_RST_POWERON &&
                              resetReason != ESP_RST_EXT &&
                              resetReason != ESP_RST_SW);

  Serial << "Reset reason: " << resetReasonName(resetReason)
         << (abnormalReset ? "   *** previous run ended abnormally ***" : "") << endl;

  // Always checked: a crash in the field is usually followed by a power-on.
  reportCoreDump();

  // Allow every log level at run time. Which levels are compiled in is chosen with
  // Tools -> Core Debug Level in the Arduino IDE (None ... Verbose).
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  // The ADC settings are pinned rather than inherited: the joysticks' adcMax (4095) assumes 12-bit
  // conversions, and the joystick travel assumes the ~3.3 V span that 11 dB attenuation gives.
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Always set the pins explicitly. The first begin() of a bus wins and later ones are ignored, so a
  // library calling a bare SPI.begin() cannot claim the default pins (MISO = GPIO 19, the I2S LRC).
  // 400 kHz rather than the 100 kHz Wire.begin() substitutes when given no frequency. The MPU-6050
  // read is 153 bit-times: 1.53 ms at 100 kHz against 0.38 ms here, in a 33 ms frame at 30 fps.
  Wire.begin(cI2C_SDA, cI2C_SCL, 400000);
  SPI.begin(cSPI_SCK, cSPI_MISO, cSPI_MOSI, cSPI_SS_CS);

  // ---- Configuration

  hasOneJoystick = DeviceTypeConfiguration::getSetup();

  // ---- Fuel Gauge

  startupStatus.fuelGauge = fuelGauge.begin();
  if (startupStatus.fuelGauge == false)
  {
    Serial << "ERROR: " << __PRETTY_FUNCTION__
           << " -> MAX17048 did not respond (is a battery plugged in?)" << endl;
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

  if (startupStatus.fuelGauge == true)
    ui.addDisplayObject(new BatteryStatus(ui, fuelGauge));
  ui.addDisplayObject(new ReceiverStatus(ui, *transmitter));
  ui.addDisplayObject(new SwitchesStatus(ui));
  if (startupStatus.imu == true)
    ui.addDisplayObject(new IMUStatus(ui, imu));
  ui.addDisplayObject(new JoystickStatus(ui, true));
  ui.addDisplayObject(new JoystickStatus(ui, false));

  // FPSStatus reads the FPS toggle, which the menu owns, so the menu and the toggle come first.
  // ~UserInterface deletes the display objects before the menu, so the reference never dangles.
  Menu* m = new Menu(ui);
  FPSToggle* fpsToggle = new FPSToggle(ui, *m);
  ui.addDisplayObject(new FPSStatus(ui, *fpsToggle));

  m->add(new WipeAndReset(ui, *m));
  m->add(new JoystickCalibration(ui, *m));
  if (startupStatus.imu == true)
    m->add(new IMUCalibration(ui, *m, imu));
  m->add(new DeviceTypeConfiguration(ui, *m));
  m->add(new SoundVolume(ui, *m));
  m->add(fpsToggle);
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

// A joystick button press is latched until a frame has carried it to the receiver. Pressed and
// read during a screen pass, sent during a later send pass.
static bool btnLeft  = false;
static bool btnRight = false;

//----------------------------------------------------------------------------------------------
// Screen pass: IMU, input, step() and draw().
static void runFrame()
{
  // previousFrameStart is what makes dt a measurement.
  static int64_t previousFrameStart = esp_timer_get_time() - frameTimeMs * 1000;

  const int64_t t0 = esp_timer_get_time();

  // Measured, not nominal - which is what step() is documented to receive. A frame that overruns
  // its budget reports the time it actually took, so anything that integrates dt stays in real
  // time instead of quietly running slow.
  const unsigned long dt = (t0 - previousFrameStart) / 1000;
  previousFrameStart = t0;

  // ---- IMU
  // Fast updates (no delays!)

  // Skipped when the IMU did not answer at startup: without this the loop drives failing I2C
  // transactions at 30 Hz forever. IMUStatus and IMUCalibration are not created in that case
  // either, so nothing downstream reads a sensor that is not there.
  if (startupStatus.imu == true)
    imu.update();

  // ---- Update the UI objects and render

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

  // Forget button presses if the menu is active or no receiver is present
  if (ui.isMenuActive() == true || transmitter->isPaired() == false)
  {
    btnLeft = btnRight = false;
  }
}

//----------------------------------------------------------------------------------------------
// Send pass: one TransmitterData frame from the input of the latest screen pass.
static void sendControls()
{
  static TransmitterData td;

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

    // Sensor orientation: X-axis points right, Y-axis points toward the top of the transmitter.
    // Without an IMU (none answered at startup) the transmitter reports level: the getters would
    // still return what MPU6050_light::begin() computed from its failed first read.
    td.angleX = (startupStatus.imu == true) ? imu.getAngleY() : 0.0f;
    td.angleY = (startupStatus.imu == true) ? imu.getAngleX() : 0.0f;
  }

  transmitter->send(td);
}

//----------------------------------------------------------------------------------------------
void loop()
{
  // When each job is next due (esp_timer time, us).
  static int64_t nextFrameUs = 0;
  static int64_t nextSendUs  = 0;

  // ---- Screen
  // A frame that overruns is not caught up: the next one is due a full frame after this one began.
  int64_t now = esp_timer_get_time();
  if (now >= nextFrameUs)
  {
    nextFrameUs = now + frameTimeMs * 1000;
    runFrame();
  }

  // ---- Link to the receiver

  // Remove the receiver if it has not sent a heartbeat for a while => communication is lost
  transmitter->removeLostPeer();

  // ---- Send data to the receiver
  // The schedule advances by the interval rather than restarting at "now", so the rate holds
  // whatever the frame time. After a longer pause (not paired, a long frame) it resynchronizes
  // instead of catching up with a burst.
  const bool paired = transmitter->isPaired();
  now = esp_timer_get_time();
  if (paired == true && now >= nextSendUs)
  {
    sendControls();

    nextSendUs += cSendIntervalMs * 1000;
    if (now >= nextSendUs)
      nextSendUs = now + cSendIntervalMs * 1000;
  }

  // ---- Sleep until the next job is due (unpaired, only the screen is due)
  // Rounded up to whole milliseconds; a wake-up that comes slightly early just sleeps again.
  const int64_t wakeUs  = (paired == true) ? min(nextFrameUs, nextSendUs) : nextFrameUs;
  const int64_t sleepUs = wakeUs - esp_timer_get_time();
  if (sleepUs > 0)
    vTaskDelay(pdMS_TO_TICKS(static_cast<uint32_t>((sleepUs + 999) / 1000)));
}
