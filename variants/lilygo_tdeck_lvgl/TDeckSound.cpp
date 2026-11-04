// T-Deck audio backend: MAX98357A I2S class-D amp straight off the ESP32-S3
// (BCK 7 / WS 5 / DOUT 6). No codec and no volume register, so the level is
// set entirely by the sample amplitude.

#include <helpers/ui/Sound.h>

#include <MeshCore.h>

#include <Arduino.h>
#include <math.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>

#define SND_I2S_BCK  7
#define SND_I2S_WS   5
#define SND_I2S_DOUT 6
#define SND_SAMPLE_RATE 44100
#define SND_AMPLITUDE 3000.0f   // the amp has fixed 9 dB gain; keep it gentle

static bool snd_ready = false;
static void (*snd_amp_ctl)(bool) = NULL;

void soundSetAmpControl(void (*fn)(bool on)) { snd_amp_ctl = fn; }

void soundInit() {
  // i2s_driver_install crashes inside IDF's cleanup if its DMA allocation
  // fails - refuse to try without clear headroom
  if (heap_caps_get_free_size(MALLOC_CAP_DMA) < 60000) {
    MESH_DEBUG_PRINTLN("sound: low DMA heap - sound disabled this boot");
    return;
  }

  i2s_config_t i2s_cfg = {};
  i2s_cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_cfg.sample_rate = SND_SAMPLE_RATE;
  i2s_cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_cfg.intr_alloc_flags = 0;
  i2s_cfg.dma_buf_count = 2;
  i2s_cfg.dma_buf_len = 256;
  i2s_cfg.tx_desc_auto_clear = true;
  if (i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, NULL) != ESP_OK) {
    MESH_DEBUG_PRINTLN("sound: i2s driver install failed");
    return;
  }
  i2s_pin_config_t pin_cfg = {};
  pin_cfg.mck_io_num = I2S_PIN_NO_CHANGE;
  pin_cfg.bck_io_num = SND_I2S_BCK;
  pin_cfg.ws_io_num = SND_I2S_WS;
  pin_cfg.data_out_num = SND_I2S_DOUT;
  pin_cfg.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(I2S_NUM_0, &pin_cfg) != ESP_OK) {
    MESH_DEBUG_PRINTLN("sound: i2s pin config failed");
    return;
  }
  i2s_stop(I2S_NUM_0);   // clocks off until a tone plays; the amp idles silent
  snd_ready = true;
  MESH_DEBUG_PRINTLN("sound: MAX98357A + I2S ready");
}

bool soundReady() { return snd_ready; }

void soundBeep(int freq_hz, int duration_ms) {
  if (!snd_ready) return;
  if (snd_amp_ctl) snd_amp_ctl(true);
  i2s_start(I2S_NUM_0);
  const int total = SND_SAMPLE_RATE * duration_ms / 1000;
  static int16_t buf[256 * 2];  // 256 stereo frames per chunk

  int written_frames = 0;
  while (written_frames < total) {
    int chunk = total - written_frames;
    if (chunk > 256) chunk = 256;
    for (int i = 0; i < chunk; i++) {
      int pos = written_frames + i;
      float t = (float) pos / SND_SAMPLE_RATE;
      // short attack/decay envelope so the beep doesn't click
      float env = 1.0f;
      if (pos < 220) env = pos / 220.0f;
      else if (pos > total - 220) env = (total - pos) / 220.0f;
      int16_t s = (int16_t)(SND_AMPLITUDE * env * sinf(2.0f * (float) M_PI * freq_hz * t));
      buf[i * 2] = s;
      buf[i * 2 + 1] = s;
    }
    size_t written_bytes = 0;
    i2s_write(I2S_NUM_0, buf, chunk * 2 * sizeof(int16_t), &written_bytes, portMAX_DELAY);
    written_frames += chunk;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_stop(I2S_NUM_0);
  if (snd_amp_ctl) snd_amp_ctl(false);
}
