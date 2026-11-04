#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/radiolib/CustomLR2021Wrapper.h>
#include "MeshPagerX2Board.h"
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/EnvironmentSensorManager.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>
#include <Adafruit_SPA06_003.h>

// Stock environment sensors plus the on-board SPA06-003 barometer (I2C1, 0x77),
// which the generic manager doesn't know about
class MeshPagerX2SensorManager : public EnvironmentSensorManager {
  Adafruit_SPA06_003 spa06;
  bool baro_ok = false;
public:
  MeshPagerX2SensorManager(LocationProvider& location) : EnvironmentSensorManager(location) { }
  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
};

extern MeshPagerX2Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern MeshPagerX2SensorManager sensors;

bool radio_init();
mesh::LocalIdentity radio_new_identity();
