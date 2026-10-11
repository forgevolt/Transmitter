// Every GPIO used in the project. The display's own pins (DC, RST, backlight) are set in
// build_opt.h with the rest of the TFT_eSPI configuration; the shared SPI pins below are checked
// against it at compile time.
#pragma once

// I2S (audio)
constexpr int cI2S_DOUT = 16;  // I2S Data out pin
constexpr int cI2S_BCLK = 17;  // Bit clock
constexpr int cI2S_LRC  = 19;  // Left/right clock, also called frame clock or word select

// I2C (IMU + fuel gauge)
constexpr int cI2C_SDA = 21;
constexpr int cI2C_SCL = 22;

// SPI
// The ESP32 Dev Module's default MISO is GPIO 19, the I2S LRC here. Safe, because setup() and
// TFT_eSPI pass explicit pins with MISO = -1, and setup() calls SPI.begin() before any library can.
constexpr int cSPI_SCK   = 18;
constexpr int cSPI_MISO  = -1; // Not used
constexpr int cSPI_MOSI  = 23; // the display module's SDA pin
constexpr int cSPI_SS_CS = 5;

// TFT_eSPI is configured by -D flags in build_opt.h: TFT_DC (15), TFT_RST (4) and TFT_BL (2) are
// set only there, and its SPI pins must equal the ones above. The check is skipped when TFT_eSPI is
// configured the conventional way instead (software/Config/TFT_eSPI), as the flags are then absent.
#if defined(TFT_SCLK) && defined(TFT_MISO) && defined(TFT_MOSI) && defined(TFT_CS)
static_assert(cSPI_SCK   == TFT_SCLK, "cSPI_SCK differs from TFT_SCLK in build_opt.h");
static_assert(cSPI_MISO  == TFT_MISO, "cSPI_MISO differs from TFT_MISO in build_opt.h");
static_assert(cSPI_MOSI  == TFT_MOSI, "cSPI_MOSI differs from TFT_MOSI in build_opt.h");
static_assert(cSPI_SS_CS == TFT_CS,   "cSPI_SS_CS differs from TFT_CS in build_opt.h");
#endif

// Switches and joysticks
constexpr int cPinJoyLBtn  = 25;
constexpr int cPinJoyRBtn  = 26;

constexpr int cPinJoyLX = 36;
constexpr int cPinJoyLY = 39;
constexpr int cPinJoyLZ = 34;

constexpr int cPinJoyRX = 33;
constexpr int cPinJoyRY = 32;
constexpr int cPinJoyRZ = 35;

constexpr uint8_t cPinS1 = 27;
constexpr uint8_t cPinS2 = 14;

// GPIO 12 is the MTDI strapping pin: its level is latched at reset and selects the flash
// regulator voltage, where high means 1.8 V. A WROOM-32E has 3.3 V flash and will not boot if
// this pin is high at reset. Safe here because the switch only ever pulls the line down - open,
// the pin sees the chip's internal pull-down; closed, it sees ground. The INPUT_PULLUP set in
// Switches::begin() is enabled from setup(), long after the latch has sampled.
// Do not fit an external pull-up to S3.
constexpr uint8_t cPinS3 = 12;

constexpr uint8_t cPinS4 = 13;

