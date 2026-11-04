# LilyGo T-Deck / T-Deck Plus — LVGL UI

The NodakMesh touch UI from the Wio Tracker L2 Pro, on the T-Deck's 320x240
screen. Upstream's `lilygo_tdeck` variant (the scaled classic UI) is unchanged
and still builds alongside this one.

| Env | What it is |
|---|---|
| `LilyGo_TDeck_standalone_lvgl` | Touch + keyboard UI, USB CLI, no Bluetooth (same as the L2 standalone build) |

## What works how

- **Touch:** GT911 on I2C 0x5D, through LovyanGFX like the L2.
- **Keyboard:** keys type into whichever field is being edited (the one with
  the on-screen keyboard up). Enter = the ✓ key, Backspace deletes.
- **Trackball click:** screen lock on/off, same as the L2's WAKE button.
  The trackball's rolling directions aren't used yet.
- **Speaker:** alert tones through the MAX98357A I2S amp.
- **SD card:** map tiles, backups and the GPS log, over the SPI bus it shares
  with the screen and radio.
- **GPS:** T-Deck Plus only, found automatically at boot (38400 baud on 43/44).

## Not tested on hardware yet — check first

1. **Touch lines up with the screen.** If taps land rotated or mirrored, change
   `TDECK_TOUCH_ROTATION` (0-7) in `TDeckLvglDisplay.h`. If touch is dead, try
   GT911 address 0x14 instead of 0x5D.
2. **Screen and radio share the SPI bus cleanly.** Watch for garbled pixels or
   radio init failures while the UI redraws.
3. **Backlight steps** — brightness uses LilyGo's 16-step pulse scheme on GPIO42.
4. **Battery reading** against a meter.
