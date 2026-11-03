#pragma once

#include <lvgl.h>

class UITask;

// Overlays pad their content by 8 px. Keyboards align to the bottom edge with
// this offset so they sit on the screen edge rather than 8 px above it, which
// leaves the band above them clear for the field being typed into.
#define KB_PAD_OFFSET 8

// On-device node & radio configuration rows for the Settings tab:
// node name, radio params (freq/BW/SF/CR with preset prefills), TX power,
// BLE pin. Applies through the same internals the phone commands use.
void settingsBuildExtras(lv_obj_t* parent, UITask* task);
void settingsRefreshRows();
int  settingsTzOffset();   // UTC offset in hours (persisted)

// exports for the first-boot wizard and about screen
int  settingsPresetCount();
const char* settingsPresetLabel(int idx);
void settingsPresetDescribe(int idx, char* buf, size_t len);   // "910.525 MHz  BW62.5 SF7"
int  settingsPresetMatch();            // preset matching the current radio params, or -1
void settingsApplyPreset(int idx);     // sets + saves radio params, no confirm
void settingsSetTzOffset(int hours);
