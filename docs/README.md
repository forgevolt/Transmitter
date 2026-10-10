# Documentation and images

| Path | Contents |
|---|---|
| `transmitter-1joy.jpg`, `transmitter-2joy.jpg` | The two photographs the top-level README shows |
| `images/UI/` | Source PNGs for the display images compiled into the firmware |
| `images/` | Other artwork - the hexapod and unknown-receiver icons at their original size |

## Display images

`images/UI/` holds the originals behind the byte arrays in `software/Transmitter/images/`. The
firmware cannot read PNGs; each one is converted to a `const unsigned short[]` header, and the
generator's URL is in the comment at the top of every generated file. Keeping the PNGs here is
what makes it possible to change an icon later - the headers themselves are not editable by hand.

| PNG | Header | Shown |
|---|---|---|
| `img1Joy.png` | `img1Joy.h` | the one-joystick device in the setup menu |
| `img2Joy.png` | `img2Joy.h` | the two-joystick device in the setup menu |
| `imgHexapod.png` | `imgHexapod.h` | a paired hexapod |
| `imgUnknowReceiver.png` | `imgUnknownReceiver.h` | a paired robot that has no dedicated icon |

## Wiring

The wiring diagram is `hardware/schematic.pdf`. `software/Transmitter/PinMap.h` lists every pin
assignment, except the display's DC, RST and backlight pins, which `software/Transmitter/build_opt.h`
sets together with the rest of the TFT_eSPI configuration.

Two details of the switch wiring are worth stating explicitly, because neither is visible in a
pin list.

**The four switches connect to ground with no external resistors.** `Switches::begin()` enables
the ESP32's internal pull-up on each pin, so a switch reads LOW closed and HIGH open - and the
firmware treats HIGH as "on".

**S3 is on GPIO 12, which constrains that wiring.** GPIO 12 is the MTDI strapping pin: its level
is latched at reset and selects the flash regulator voltage, high meaning 1.8 V. A WROOM-32E has
3.3 V flash and will not boot if GPIO 12 is high at reset. The design is safe because the switch
can only pull the line down - open, the pin sees the chip's internal pull-down; closed, it sees
ground. **Do not fit an external pull-up to S3.** It is an otherwise reasonable thing to add,
and it would stop the board booting.

This is specific to modules with 3.3 V flash. ESP32-WROVER modules other than the -B run 1.8 V
flash and carry an internal pull-up on GPIO 12, where the same wiring behaves differently.

GPIO 12 is one of four strapping pins in use; `hardware/README.md` lists all of them with the level
each needs at reset.
