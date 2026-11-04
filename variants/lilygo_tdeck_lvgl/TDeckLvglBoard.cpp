#include "TDeckLvglBoard.h"

void TDeckLvglBoard::begin() {
  // peripheral rail first: keyboard, touch, GNSS and the TFT all hang off it
  pinMode(TDECK_PIN_POWERON, OUTPUT);
  digitalWrite(TDECK_PIN_POWERON, HIGH);

  // shared SPI bus: deselect every device before any driver claims the bus
  for (uint8_t cs : { (uint8_t) TDECK_PIN_TFT_CS, (uint8_t) P_LORA_NSS, (uint8_t) TDECK_PIN_SD_CS }) {
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
  }
  pinMode(P_LORA_MISO, INPUT_PULLUP);

  ESP32Board::begin();  // starts Wire on PIN_BOARD_SDA/PIN_BOARD_SCL (18/8)

  pinMode(TDECK_PIN_TB_CLICK, INPUT_PULLUP);
  pinMode(TDECK_PIN_BAT_ADC, INPUT);

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_DEEPSLEEP) {
    long wakeup_source = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_source & (1 << P_LORA_DIO_1)) {
      startup_reason = BD_STARTUP_RX_PACKET;  // received a LoRa packet while asleep
    }
    rtc_gpio_hold_dis((gpio_num_t) P_LORA_NSS);
    rtc_gpio_deinit((gpio_num_t) P_LORA_DIO_1);
  }
}

uint16_t TDeckLvglBoard::getBattMilliVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) {
    mv += analogReadMilliVolts(TDECK_PIN_BAT_ADC);
  }
  return (uint16_t) ((mv / 8) * 2);   // 1:1 divider on the battery sense
}

uint8_t TDeckLvglBoard::readKeyboard() {
  if (Wire.requestFrom((uint8_t) TDECK_KB_ADDR, (uint8_t) 1) != 1) return 0;
  return (uint8_t) Wire.read();
}
