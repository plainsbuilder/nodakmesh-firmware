#pragma once

// Audio for the LVGL UI. Each variant supplies the backend (soundInit,
// soundSetAmpControl, soundReady, soundBeep); the alert tones and their saved
// style live in ui-lvgl/SoundTones.cpp. Everything is a safe no-op if the
// backend failed to initialize.

#define SOUND_TONE_STYLES 6   // Classic, Chirp, Ding dong, Trill, Rise, Alarm

// --- variant backend ---
void soundInit();          // call once after Wire is up (board.begin done)
void soundSetAmpControl(void (*fn)(bool on));   // board hook to gate the speaker amp power
bool soundReady();
void soundBeep(int freq_hz, int duration_ms);   // short blocking tone

// --- shared tones (ui-lvgl/SoundTones.cpp) ---
void soundMessageTone();   // incoming DM (plays the selected alert style)
void soundChannelTone();   // incoming channel message (softer variant)
void soundAckTone();       // delivery ack / confirm

void soundLoadTonePref();          // read /tone from SPIFFS (safe before soundInit)
void soundSetToneStyle(int style); // select + persist an alert style
int  soundGetToneStyle();
