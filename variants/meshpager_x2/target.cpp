#include <Arduino.h>
#include "target.h"

MeshPagerX2Board board;

// LR2021 reset is on the IO expander, so RadioLib gets no reset pin;
// board.loraReset() pulses it instead
static SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, RADIOLIB_NC, P_LORA_BUSY, spi);

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider gps(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(gps);

// Wio-LR2021 RF switch on DIO5/DIO6, from Seeed's BSP (ral_lr20xx_bsp.c)
static const uint32_t rfswitch_dio_pins[Module::RFSWITCH_MAX_PINS] = {
  RADIOLIB_LR2021_DIO5, RADIOLIB_LR2021_DIO6, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC,
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode                 DIO5  DIO6
  { LR2021::MODE_STBY,  { LOW,  LOW  } },
  { LR2021::MODE_RX,    { HIGH, HIGH } },   // sub-GHz RX
  { LR2021::MODE_TX,    { LOW,  HIGH } },   // sub-GHz TX
  { LR2021::MODE_RX_HF, { LOW,  HIGH } },   // 2.4 GHz RX
  { LR2021::MODE_TX_HF, { HIGH, HIGH } },   // 2.4 GHz TX
  END_OF_MODE_TABLE,
};

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);   // YSN8900E sits on I2C0 with the expander

  board.loraReset();
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
  bool ok = radio.std_init(&spi);
  MESH_DEBUG_PRINTLN("radio_init: LR2021 %s", ok ? "OK" : "FAILED");
  return ok;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
