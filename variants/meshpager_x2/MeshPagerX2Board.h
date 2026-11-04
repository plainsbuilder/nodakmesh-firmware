#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "helpers/ESP32Board.h"

// ---------------------------------------------------------------------------
// Seeed SenseCAP MeshPager X2  (ESP32-S3R8 + Wio-LR2021 + Airoha AG3335)
//
// Power rails, resets and the d-pad all sit behind a TCA6424 24-bit I2C IO
// expander at 0x22 on I2C0 (GPIO17/18). The board stays on only while the
// expander drives PWR_HOLD high, so begin() must run before anything else.
//
// Pin map from Seeed's hardware block diagram and schematic (rev V01);
// power-up order from Seeed's Meshtastic port (meshtastic/firmware PR #11255).
// Alpha hardware: verify against the unit before trusting any of this.
// ---------------------------------------------------------------------------

#define TCA6424_ADDR   0x22

// TCA6424 linear pin numbering: port * 8 + bit (P00 = 0 ... P27 = 23)
#define EXP_PIN_BTN_UP        0   // input  - d-pad up (active low)
#define EXP_PIN_BTN_DOWN      1   // input  - d-pad down
#define EXP_PIN_BTN_LEFT      2   // input  - d-pad left
#define EXP_PIN_BTN_RIGHT     3   // input  - d-pad right
#define EXP_PIN_BTN_CONFIRM   4   // input  - d-pad centre
#define EXP_PIN_UNUSED_P05    5   // input  - test point only
#define EXP_PIN_CHG_STAT      6   // input  - BQ25302 charge status
#define EXP_PIN_SD_DETECT     7   // input  - microSD card detect
#define EXP_PIN_GNSS_RTC_INT  8   // output - AG3335 RTC wake
#define EXP_PIN_GNSS_SLEEP    9   // output - AG3335 sleep control
#define EXP_PIN_IMU_INT1     10   // input  - LSM6DSO INT1
#define EXP_PIN_SEN_EN       12   // output - sensor rail + audio codec LDO
#define EXP_PIN_SD_EN        13   // output - microSD rail
#define EXP_PIN_LCD_EN       14   // output - display rail
#define EXP_PIN_GNSS_VRTC_EN 15   // output - GNSS backup supply
#define EXP_PIN_GNSS_EN      16   // output - GNSS rail
#define EXP_PIN_BAT_ADC_EN   17   // output - battery divider enable
#define EXP_PIN_AUDIO_PA_EN  18   // output - speaker amp rail
#define EXP_PIN_USB_IN       19   // input  - VBUS present
#define EXP_PIN_PWR_HOLD     20   // output - power latch, must stay high
#define EXP_PIN_LCD_RST      21   // output - display reset (active low)
#define EXP_PIN_LORA_RST     22   // output - LR2021 reset (active low)
#define EXP_PIN_GNSS_RST     23   // output - GNSS reset (active high pulse)

#define X2_DPAD_MASK 0x1F         // P00..P04
#define PIN_RETURN_BTN 0          // Return key shares GPIO0 with BOOT, active low

class MeshPagerX2Board : public ESP32Board {
public:
  void begin();

  uint16_t getBattMilliVolts() override;
  bool isExternalPowered() override;
  void powerOff() override;

  const char* getManufacturerName() const override {
    return "Seeed SenseCAP MeshPager X2";
  }

  // pulse the LR2021 reset line; call right before radio init
  void loraReset();

  void setGnssPower(bool on);
  void setDisplayPower(bool on);

  // d-pad bits (1 = pressed) in X2_DPAD_MASK order, or -1 if the read failed
  int readDpad();

  bool expanderOK() const { return expander_ok; }

private:
  uint8_t out_shadow[3] = { 0x00, 0x00, 0x00 };
  uint8_t cfg_shadow[3] = { 0xFF, 0xFF, 0xFF };  // 1 = input (power-on default)
  bool expander_ok = false;

  bool expReadReg(uint8_t reg, uint8_t& val);
  bool expWriteReg(uint8_t reg, uint8_t val);
  bool expSetOutput(uint8_t pin, bool initial_level);
  bool expSetInput(uint8_t pin);
  bool expWritePin(uint8_t pin, bool level);
  int  expReadPin(uint8_t pin);
  bool initExpander();
};
