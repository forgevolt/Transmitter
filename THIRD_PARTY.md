# Third-party licences

The Transmitter firmware is MIT licensed (see LICENSE). It builds against the libraries below.
None of them are redistributed here - each is installed through the Arduino Library Manager and
keeps its own licence.

| Library           | Licence                                  | Used for                                     |
| ----------------- | ---------------------------------------- | -------------------------------------------- |
| TFT_eSPI          | FreeBSD; Adafruit-derived parts MIT, BSD | the 480x320 display                          |
| Bounce2           | MIT                                      | debouncing the joystick buttons and switches |
| MPU6050_light     | MIT                                      | the IMU                                      |
| Adafruit MAX1704X | BSD                                      | the battery fuel gauge                       |
| Adafruit BusIO    | MIT                                      | I2C access for the fuel gauge                |
| Streaming         | LGPL-2.1-or-later                        | the `Serial <<` logging                      |
| ESPNowUtilities   | MIT                                      | the ESP-NOW link to the robot                |
| JoystickUtilities | MIT                                      | reading and calibrating the joysticks        |
| SoundEngine       | MIT                                      | I2S playback and the built-in sounds         |
| VectorUtilities   | MIT (raymath: zlib)                      | `mapf()`, `deg2rad`, the IMU disk mapping    |

The last four are written for this project and published separately, under the same MIT
licence as this repository:
[ESPNowUtilities](https://github.com/forgevolt/ESPNowUtilities),
[JoystickUtilities](https://github.com/forgevolt/JoystickUtilities),
[SoundEngine](https://github.com/forgevolt/SoundEngine) and
[VectorUtilities](https://github.com/forgevolt/VectorUtilities). VectorUtilities' vector code is
derived from raymath, part of [raylib](https://github.com/raysan5/raylib) (zlib licence); the
library carries raylib's licence notice.

## Streaming

`Streaming.h` by Mikal Hart is used for logging throughout the firmware. The LGPL does not
affect the licence of this project, but it asks that anyone who receives a binary can relink
it against a modified version of the library. Complete source is published here and no
pre-built binaries are distributed, so a recipient can modify Streaming and rebuild.

## TFT_eSPI configuration files

`software/Config/TFT_eSPI/User_Setup_Select.h` and
`software/Config/TFT_eSPI/User_Setups/Setup27_ST7796_ESP32_320x480.h` are TFT_eSPI's own
configuration files by Bodmer, adapted to this display. They remain under TFT_eSPI's licence
(the library's `license.txt`), not this repository's MIT licence, and are here only for
configuring the library the conventional way - see `software/Config/TFT_eSPI/README.md`. The
build itself does not need them: `software/Transmitter/build_opt.h` carries the same settings as
compiler flags.

## Display images

The headers in `software/Transmitter/images/` are generated from this project's own PNGs
(`docs/images/UI/`); the converter is named at the top of each file. They are covered by this
repository's licence.
