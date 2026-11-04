// Alert tones for the LVGL UI, played through the variant's soundBeep().

#include <helpers/ui/Sound.h>

#include <Arduino.h>
#include <SPIFFS.h>

// selectable alert melodies; {0,0} terminates a style early
struct ToneNote { uint16_t freq; uint16_t ms; };
static const ToneNote TONE_STYLES[SOUND_TONE_STYLES][5] = {
  {{880, 90}, {1175, 120}, {0, 0}},                          // Classic
  {{1400, 40}, {1800, 40}, {2200, 60}, {0, 0}},              // Chirp
  {{1319, 140}, {988, 180}, {0, 0}},                         // Ding dong
  {{1568, 45}, {1245, 45}, {1568, 45}, {1245, 90}, {0, 0}},  // Trill
  {{659, 70}, {880, 70}, {1175, 70}, {1568, 110}, {0, 0}},   // Rise
  {{1047, 120}, {523, 120}, {1047, 120}, {0, 0}},            // Alarm
};
static int tone_style = 0;

void soundLoadTonePref() {
  File f = SPIFFS.open("/tone", "r");
  if (f) {
    int v = f.parseInt();
    f.close();
    if (v >= 0 && v < SOUND_TONE_STYLES) tone_style = v;
  }
}

void soundSetToneStyle(int style) {
  if (style < 0 || style >= SOUND_TONE_STYLES) return;
  tone_style = style;
  File f = SPIFFS.open("/tone", "w");
  if (f) {
    f.print(style);
    f.close();
  }
}

int soundGetToneStyle() { return tone_style; }

void soundMessageTone() {
  const ToneNote* n = TONE_STYLES[tone_style];
  for (int i = 0; i < 5 && n[i].freq != 0; i++) soundBeep(n[i].freq, n[i].ms);
}

void soundChannelTone() {
  // softer cousin of the chosen alert: first note, dropped a fourth
  soundBeep(TONE_STYLES[tone_style][0].freq * 3 / 4, 100);
}

void soundAckTone()     { soundBeep(1568, 50); }
