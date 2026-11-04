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
MeshPagerX2SensorManager sensors(gps);

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

bool MeshPagerX2SensorManager::begin() {
  bool ok = EnvironmentSensorManager::begin();   // also starts Wire1 on 47/48

  baro_ok = spa06.begin(SPA06_003_DEFAULT_ADDR, &Wire1);
  if (baro_ok) {
    spa06.setPressureOversampling(SPA06_003_OVERSAMPLE_8);
    spa06.setTemperatureOversampling(SPA06_003_OVERSAMPLE_8);
    // 1 Hz continuous keeps reads non-blocking at minimal power cost
    spa06.setPressureMeasureRate(SPA06_003_RATE_1);
    spa06.setTemperatureMeasureRate(SPA06_003_RATE_1);
    spa06.setMeasurementMode(SPA06_003_MEAS_CONTINUOUS_BOTH);
  }
  MESH_DEBUG_PRINTLN("sensors: SPA06 %s", baro_ok ? "OK" : "not found");
  return ok;
}

bool MeshPagerX2SensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  EnvironmentSensorManager::querySensors(requester_permissions, telemetry);
  if ((requester_permissions & TELEM_PERM_ENVIRONMENT) && baro_ok) {
    telemetry.addTemperature(TELEM_CHANNEL_SELF, spa06.readTemperature());
    telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, spa06.readPressure());
  }
  return true;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
