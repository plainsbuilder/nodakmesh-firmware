#pragma once

#include <helpers/ui/LGFXDisplay.h>
#include <Wire.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ---------------------------------------------------------------------------
// T-Deck display stack:
//   - ST7789 320x240 on the shared SPI bus (SCK 40 / MOSI 41 / MISO 38,
//     CS 12, DC 11); bus_shared so the radio and SD card get their turn
//   - GT911 capacitive touch on I2C 0x5D (SDA 18 / SCL 8, INT 16)
//   - backlight driver on GPIO42 that dims by counting pulses (16 steps),
//     so plain PWM would make it hunt between levels
// ---------------------------------------------------------------------------

#ifndef TDECK_SPI_FREQUENCY
  #define TDECK_SPI_FREQUENCY 40000000
#endif
#ifndef TDECK_TOUCH_ROTATION
  #define TDECK_TOUCH_ROTATION 0   // verify on hardware: GT911 vs panel orientation
#endif

#define TDECK_PIN_BL 42

// LilyGo's backlight scheme: each low pulse steps the driver one level down,
// wrapping at 16. Brightness 0-255 maps onto those 16 steps.
class TDeckBacklight : public lgfx::v1::ILight {
  static constexpr uint8_t STEPS = 16;
  uint8_t _level = 0;          // current driver step, 0 = off
  uint8_t _brightness = 153;   // last requested 0-255 value

public:
  bool init(uint8_t brightness) override {
    pinMode(TDECK_PIN_BL, OUTPUT);
    digitalWrite(TDECK_PIN_BL, LOW);
    _level = 0;
    setBrightness(brightness);
    return true;
  }

  void setBrightness(uint8_t brightness) override {
    _brightness = brightness;
    uint8_t value = (uint8_t) ((brightness * STEPS + 254) / 255);   // 1-255 -> 1-16
    if (value == 0) {
      digitalWrite(TDECK_PIN_BL, LOW);
      delay(3);   // held low this long, the driver resets to off
      _level = 0;
      return;
    }
    if (_level == 0) {
      digitalWrite(TDECK_PIN_BL, HIGH);   // power-up lands on full brightness
      _level = STEPS;
      delayMicroseconds(30);
    }
    int from = STEPS - _level;
    int to = STEPS - value;
    int pulses = (STEPS + to - from) % STEPS;
    for (int i = 0; i < pulses; i++) {
      digitalWrite(TDECK_PIN_BL, LOW);
      digitalWrite(TDECK_PIN_BL, HIGH);
    }
    _level = value;
  }

  uint8_t getBrightness() const { return _brightness; }

  virtual ~TDeckBacklight() = default;
};

class LGFX_TDeck : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Touch_GT911 _touch_instance;
  TDeckBacklight _light_instance;

public:
  bool init_impl(bool use_reset, bool use_clear) override {
    bool result = LGFX_Device::init_impl(use_reset, use_clear);

    // the GT911 probe drives I2C port 0 behind Arduino's back and can leave
    // its BUSY flag stuck; cycling Wire resets it for the keyboard and sensors
    Wire.end();
    Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);

    return result;
  }

  LGFX_TDeck(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;   // same host as Arduino's SPI (radio, SD)
      cfg.spi_mode = 0;
      cfg.freq_write = TDECK_SPI_FREQUENCY;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 40;
      cfg.pin_mosi = 41;
      cfg.pin_miso = 38;
      cfg.pin_dc = 11;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 12;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;   // native portrait orientation
      cfg.panel_height = 320;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 1;  // keyboard below the screen: landscape 320x240
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.readable = false;
      cfg.bus_shared = true;    // release the bus between frames
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _touch_instance.config();
      cfg.pin_cs = -1;
      cfg.x_min = 0;
      cfg.x_max = 239;
      cfg.y_min = 0;
      cfg.y_max = 319;
      cfg.pin_int = 16;
      cfg.offset_rotation = TDECK_TOUCH_ROTATION;
      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x5D;
      cfg.pin_sda = PIN_BOARD_SDA;
      cfg.pin_scl = PIN_BOARD_SCL;
      cfg.bus_shared = true;
      cfg.freq = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    _panel_instance.setLight(&_light_instance);
    setPanel(&_panel_instance);
  }
};

class TDeckLvglDisplay : public LGFXDisplay {
  LGFX_TDeck disp;
public:
  TDeckLvglDisplay() : LGFXDisplay(320, 240, disp) {}

  // direct access to the LGFX device (LVGL flush/touch glue)
  lgfx::LGFX_Device* lgfxDevice() { return &disp; }

  // shadows LGFXDisplay::begin() (non-virtual, called on the concrete type in
  // main.cpp): the panel config already carries the landscape rotation
  bool begin() {
    turnOn();
    display->init();
    display->setRotation(0);
    display->setColorDepth(8);
    display->fillScreen(TFT_BLACK);
    display->setBrightness(153);
    display->setTextColor(TFT_WHITE);

    buffer.setColorDepth(8);
    buffer.setPsram(true);
    buffer.createSprite(width(), height());

    return true;
  }
};
