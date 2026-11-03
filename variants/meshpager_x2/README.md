# Seeed SenseCAP MeshPager X2 (alpha)

ESP32-S3R8 (8 MB PSRAM) + Wio-LR2021 + Airoha AG3335 GNSS, 2" ST7789P3
320x240 display, 5-way d-pad, TCA6424 IO expander.

Pin map from the board schematic (rev V01) cross-checked against Seeed's
Meshtastic port, [meshtastic/firmware#11255](https://github.com/meshtastic/firmware/pull/11255).
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
6. **Sources disagree on:**
   - Return key: expander P05 (Meshtastic) vs GPIO0 (schematic)
   - Expander P06: power button (Meshtastic) vs charger status (schematic)
   - SHT4x temperature/humidity (Meshtastic) vs SPA06 pressure (schematic, 0x77)
   - Flash: 16 MB layout used here, schematic shows 32 MB
