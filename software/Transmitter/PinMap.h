// Single source of truth for every GPIO used in the project
#pragma once

// I2S (audio)
constexpr int cI2S_DOUT = 16;  // I2S Data out pin
constexpr int cI2S_BCLK = 17;  // Bit clock
constexpr int cI2S_LRC  = 19;  // Left/Right clock, also known as Frame clock or word select

// I2C (display + IMU)
constexpr int cI2C_SDA = 21;
constexpr int cI2C_SCL = 22;

// SPI
constexpr int cSPI_SCK   = 18;
constexpr int cSPI_MISO  = -1; // Not used
constexpr int cSPI_MOSI  = 23; // SDA
constexpr int cSPI_SS_CS = 5; 

// TFT_eSPI configuration
// #define TFT_MISO -1
// #define TFT_MOSI 23
// #define TFT_SCLK 18
// #define TFT_CS    5
// #define TFT_DC   15
// #define TFT_RST   4 

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

