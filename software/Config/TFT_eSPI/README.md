# TFT_eSPI configuration

TFT_eSPI is configured at compile time, not at runtime: the display driver and the pin
assignments are `#define`s that the library reads when it is compiled. A stock TFT_eSPI
therefore will not drive this transmitter's panel — it has to be told which driver and which
pins to use.

There are two ways to do that. The project ships both, and **you only need one**.

## The short way: `build_opt.h` (nothing to install)

`Transmitter/build_opt.h` already carries the whole configuration. The ESP32 board package
copies any `build_opt.h` found in the sketch folder into the build directory and passes it to
the compiler as a response file, so its flags apply to the sketch *and* to every library
compiled alongside it — TFT_eSPI included.

Nothing to do. Open `Transmitter/Transmitter.ino` and build. `-DUSER_SETUP_LOADED` tells
TFT_eSPI its own `User_Setup.h` is not wanted, and the rest of the flags replace it.

Two things to know if you edit that file:

- **It is a compiler response file, not a header**, despite the `.h`. It may contain compiler
  flags and nothing else. A `//` comment is passed to the linker as a filename and the build
  fails with `cannot find //: file format not recognized`.
- It is specific to the ESP32 Arduino core. Other cores ignore it, which is what the second
  route is for.

## The conventional way: copy the setup into the library

This is how TFT_eSPI documents its own configuration, and it is the route to take if you are
not on the ESP32 Arduino core or you prefer the library configured in the usual place.

Copy both of these into your TFT_eSPI installation, overwriting what is there:

| From here | To |
|---|---|
| `User_Setup_Select.h` | `<libraries>/TFT_eSPI/User_Setup_Select.h` |
| `User_Setups/Setup27_ST7796_ESP32_320x480.h` | `<libraries>/TFT_eSPI/User_Setups/Setup27_ST7796_ESP32_320x480.h` |

`<libraries>` is `~/Documents/Arduino/libraries` on macOS and Linux, and
`Documents\Arduino\libraries` on Windows.

Then **delete `Transmitter/build_opt.h`**, or the two will both be active. They currently hold
the same values, so nothing would break — but a later edit to one and not the other gives you a
display that works until it silently doesn't, and the flags win, which is the opposite of where
you would look.

Note that this route is undone by a TFT_eSPI update: the library manager replaces the whole
folder, including these two files. That is the reason `build_opt.h` is the recommended route.

## What the configuration says

| Setting | Value |
|---|---|
| Driver | ST7796, 320x480 |
| MISO | not connected (`-1`) |
| MOSI | 23 |
| SCLK | 18 |
| CS | 5 |
| DC | 15 |
| RST | 4 |
| Backlight | 2, active high |
| SPI | 80 MHz |
| Fonts | GLCD, 2, 4, 6, 7, 8, smooth |

The pins are the same ones recorded in `Transmitter/PinMap.h`, which is the single source of
truth for every GPIO in the project. If you rewire the display, change both — `PinMap.h` documents
it, and one of the two routes above is what actually reaches the library.
