#pragma once

// SD card backend for the UI, always mounted at /sdcard (LVGL's POSIX fs
// driver reads map tiles from there). Boards wire the card one of two ways:
//   - 1-bit SDIO (default): SD_PIN_CLK / SD_PIN_CMD / SD_PIN_D0
//   - SPI shared with other devices: define SD_SPI_CS; uses the global SPI bus

// PlatformIO's library finder doesn't scan this header, so envs using the UI
// list SD and SD_MMC in lib_deps explicitly
#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>

#ifdef SD_SPI_CS
  #define UI_SD SD
  #ifndef SD_SPI_FREQ
    #define SD_SPI_FREQ 20000000
  #endif
#else
  #define UI_SD SD_MMC
  #ifndef SD_PIN_CLK
    #define SD_PIN_CLK 2   // Wio Tracker L2 Pro wiring
    #define SD_PIN_CMD 3
    #define SD_PIN_D0  1
  #endif
#endif

static inline bool uiSdBegin() {
#ifdef SD_SPI_CS
  return SD.begin(SD_SPI_CS, SPI, SD_SPI_FREQ, "/sdcard");
#else
  SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
  return SD_MMC.begin("/sdcard", true /* 1-bit mode */);
#endif
}

static inline bool uiSdPresent() {
  return UI_SD.cardType() != CARD_NONE && UI_SD.cardType() != CARD_UNKNOWN;
}
