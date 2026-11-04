#include <Arduino.h>
#include "target.h"

TDeckLvglBoard board;

// the radio uses the global SPI object so it shares one Arduino bus driver
// with the SD card; the TFT reaches the same host through LovyanGFX
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
GpsTapStream gps_tap(Serial1);   // GSV sats-in-view / SNR watcher (T-Deck Plus GNSS)
MicroNMEALocationProvider gps(gps_tap, &rtc_clock);
EnvironmentSensorManager sensors(gps);

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

  bool ok = radio.std_init(&SPI);
  MESH_DEBUG_PRINTLN("radio_init: SX1262 %s", ok ? "OK" : "FAILED");
  return ok;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
