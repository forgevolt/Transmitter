# Software

| Path | What |
|---|---|
| `Transmitter/` | The firmware. This is the one you flash |
| `Config/TFT_eSPI/` | The display setup for configuring TFT_eSPI the conventional way - not needed with `build_opt.h`, see its README |

## `Transmitter/` - the firmware

| Files | Responsibility |
| --- | --- |
| `Transmitter.ino` | `setup()`: hardware init, startup check, screen objects and menu; `loop()`: the screen and send schedules |
| `Transmitter.*` | the ESP-NOW link: pairing, sending `TransmitterData`, receiving the robot's telemetry |
| `RCProtocol.h` | the wire format - the master copy; see the note in the top-level README |
| `InputController.*`, `Switches.*` | the joysticks, the two joystick buttons and the four switches |
| `UserInterface.*`, `DisplayObject.*` | screen, sound and input in one place; the base class of everything drawn |
| `BatteryStatus.*`, `ReceiverStatus.*`, `JoystickStatus.*`, `SwitchesStatus.*`, `IMUStatus.*`, `FPSStatus.*` | the widgets of the main screen |
| `Menu.*`, `MenuItem.*` | the menu, opened with SWITCH 1 |
| `JoystickCalibration.*`, `IMUCalibration.*`, `DeviceTypeConfiguration.*`, `SoundVolume.*`, `FPSToggle.*`, `WipeAndReset.*` | the menu items; settings and calibration are stored in NVS |
| `images/` | the display images as `const` arrays, generated from `docs/images/UI/` |
| `PinMap.h` | every GPIO in the project (the display's DC, RST and backlight are set in `build_opt.h`) |
| `build_opt.h` | the TFT_eSPI configuration as `-D` flags, and extra compiler warnings: `-Wall -Wextra -Wdouble-promotion -Wsign-compare` |
| `partitions.csv` | 6 MB app + 1.9 MB LittleFS; overrides the Tools menu |

Radio transport, joystick reading, audio and the vector helpers are not here - they are the
ESPNowUtilities, JoystickUtilities, SoundEngine and VectorUtilities libraries, installed through
the Library Manager.
