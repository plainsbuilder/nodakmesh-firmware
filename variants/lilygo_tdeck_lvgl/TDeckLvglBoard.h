#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "helpers/ESP32Board.h"

// ---------------------------------------------------------------------------
// LilyGo T-Deck / T-Deck Plus for the LVGL UI  (ESP32-S3 + SX1262)
//
// The TFT, LoRa radio and SD card share one SPI bus (SCK 40 / MOSI 41 /
// MISO 38), so all three chip selects are parked high before anything
// touches the bus. Keyboard (ESP32-C3 at 0x55) and GT911 touch sit on I2C
// 18/8 and only respond once the peripheral power pin (GPIO10) is high.
// ---------------------------------------------------------------------------

#define TDECK_PIN_POWERON   10
#define TDECK_PIN_BAT_ADC    4
#define TDECK_PIN_TFT_CS    12
#define TDECK_PIN_SD_CS     39
#define TDECK_PIN_TB_CLICK   0   // trackball press (also BOOT)
#define TDECK_KB_ADDR     0x55

class TDeckLvglBoard : public ESP32Board {
public:
  void begin();

  uint16_t getBattMilliVolts() override;

  const char* getManufacturerName() const override {
    return "LilyGo T-Deck";
  }

  // trackball click doubles as the screen-lock button
  bool readWakeButton() { return digitalRead(TDECK_PIN_TB_CLICK) == LOW; }

  // next ASCII key from the keyboard, or 0 if none / keyboard absent
  uint8_t readKeyboard();

  // The UI drives these on boards with switchable rails; the T-Deck powers
  // GNSS, speaker amp and expansion port straight from the peripheral rail
  void gnssReset() { }
  void setGnssPower(bool on) { }
  void setSpeakerAmp(bool on) { }
  void setGrovePower(bool on) { }
};
