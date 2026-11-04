# Seeed SenseCAP MeshPager X2 (alpha)

ESP32-S3R8 (8 MB PSRAM) + Wio-LR2021 + Airoha AG3335 GNSS, 2" ST7789P3
320x240 display, 5-way d-pad, TCA6424 IO expander.

Pin map from Seeed's hardware block diagram and the board schematic (rev V01);
power-up order from Seeed's Meshtastic port,
[meshtastic/firmware#11255](https://github.com/meshtastic/firmware/pull/11255).
None of it has run on hardware yet.

## Builds

| Env | What it is |
|---|---|
| `MeshPager_X2_companion_radio_ble` | Phone app over Bluetooth, screen off |
| `MeshPager_X2_companion_radio_usb` | Phone/desktop app over USB, screen off |
| `MeshPager_X2_repeater` | Standalone repeater |

The display stays powered down: the LVGL UI is touch-only and needs d-pad
navigation before it is useful on this board.

## Bring-up checklist

1. **Stays on after releasing the power button.** If not, PWR_HOLD (expander
   P24) is not being latched; check the TCA6424 answers at 0x22 on GPIO17/18.
2. **Radio init.** Look for `radio_init: LR2021 OK` with `MESH_DEBUG=1`.
   `-706`/`-707` means the TCXO voltage is wrong (3.0 V here, 1.6 V on the X1);
   the driver retries at 0 V, which will init but drift.
3. **TX/RX both work.** If one direction is deaf, the DIO5/DIO6 RF switch
   table in `target.cpp` is the suspect.
4. **GNSS fix.** NMEA at 115200 on GPIO44 (RX) / GPIO43 (TX).
5. **Battery reading.** GPIO1 behind a 2:1 divider; tune `ADC_MULTIPLIER`
   against a meter.
6. **Barometer.** SPA06-003 at 0x77 on I2C1 should show up in telemetry.

## Settled by the block diagram

The Meshtastic port disagreed with the schematic on a few points; Seeed's
block diagram sides with the schematic:

- Return key is GPIO0 (shared with BOOT); expander P05 is unused.
- Expander P06 is the charger STAT line, not a button.
- No SHT4x: the environment sensor is the SPA06-003 barometer.
- Flash is 32 MB (PY25Q256HB). The build still uses the 16 MB layout, which
  runs fine on it; the top half is unused.
- GNSS PPS is on GPIO21 (`PIN_GPS_PPS`, not used yet).
