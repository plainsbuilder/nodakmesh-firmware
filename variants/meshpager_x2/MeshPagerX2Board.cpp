#include "MeshPagerX2Board.h"

// TCA6424 register map: three consecutive registers per bank, one per port
static const uint8_t TCA_REG_INPUT0  = 0x00;
static const uint8_t TCA_REG_OUTPUT0 = 0x04;
static const uint8_t TCA_REG_CONFIG0 = 0x0C;  // 1 = input, 0 = output

// After a warm reboot a slave (the ES8311 codec in particular) can be left
// mid-byte holding SDA low. Clock it out and issue a STOP before Wire claims
// the pins. On a cold boot SDA is already high and this returns at once.
static void recoverI2C(uint8_t sda, uint8_t scl) {
  pinMode(scl, OUTPUT);
  digitalWrite(scl, HIGH);
  pinMode(sda, INPUT_PULLUP);
  delayMicroseconds(5);
  for (int i = 0; i < 9 && !digitalRead(sda); i++) {
    digitalWrite(scl, LOW);
    delayMicroseconds(5);
    digitalWrite(scl, HIGH);
    delayMicroseconds(5);
  }
  pinMode(sda, OUTPUT);
  digitalWrite(sda, LOW);
  delayMicroseconds(5);
  digitalWrite(sda, HIGH);
  delayMicroseconds(5);
  pinMode(scl, INPUT);
  pinMode(sda, INPUT);
}

bool MeshPagerX2Board::expReadReg(uint8_t reg, uint8_t& val) {
  Wire.beginTransmission(TCA6424_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)TCA6424_ADDR, (uint8_t)1) != 1) return false;
  val = Wire.read();
  return true;
}

bool MeshPagerX2Board::expWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TCA6424_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool MeshPagerX2Board::expSetOutput(uint8_t pin, bool initial_level) {
  uint8_t port = pin >> 3, bit = pin & 7;
  // set the level first so the pin doesn't glitch when direction flips
  if (initial_level) out_shadow[port] |= (1 << bit);
  else out_shadow[port] &= ~(1 << bit);
  if (!expWriteReg(TCA_REG_OUTPUT0 + port, out_shadow[port])) return false;

  cfg_shadow[port] &= ~(1 << bit);
  return expWriteReg(TCA_REG_CONFIG0 + port, cfg_shadow[port]);
}

bool MeshPagerX2Board::expSetInput(uint8_t pin) {
  uint8_t port = pin >> 3, bit = pin & 7;
  cfg_shadow[port] |= (1 << bit);
  return expWriteReg(TCA_REG_CONFIG0 + port, cfg_shadow[port]);
}

bool MeshPagerX2Board::expWritePin(uint8_t pin, bool level) {
  uint8_t port = pin >> 3, bit = pin & 7;
  if (level) out_shadow[port] |= (1 << bit);
  else out_shadow[port] &= ~(1 << bit);
  return expWriteReg(TCA_REG_OUTPUT0 + port, out_shadow[port]);
}

int MeshPagerX2Board::expReadPin(uint8_t pin) {
  uint8_t v;
  if (!expReadReg(TCA_REG_INPUT0 + (pin >> 3), v)) return -1;
  return (v >> (pin & 7)) & 1;
}

// Power-up order and delays mirror Seeed's BSP (bsp_power_startup_peripherals)
// as carried into the Meshtastic port. Rails are staggered to limit inrush.
bool MeshPagerX2Board::initExpander() {
  // adopt the live register state: the expander survives a CPU reset and is
  // still holding PWR_HOLD and the rails, so don't start from defaults
  for (uint8_t port = 0; port < 3; port++) {
    if (!expReadReg(TCA_REG_OUTPUT0 + port, out_shadow[port])) return false;
    if (!expReadReg(TCA_REG_CONFIG0 + port, cfg_shadow[port])) return false;
  }

  bool ok = expSetOutput(EXP_PIN_PWR_HOLD, HIGH);   // latch power first

  for (uint8_t pin = EXP_PIN_BTN_UP; pin <= EXP_PIN_SD_DETECT; pin++) {
    ok &= expSetInput(pin);
  }
  ok &= expSetInput(EXP_PIN_IMU_INT1);
  ok &= expSetInput(EXP_PIN_USB_IN);

  ok &= expSetOutput(EXP_PIN_BAT_ADC_EN, HIGH);
  delay(10);
  ok &= expSetOutput(EXP_PIN_SEN_EN, HIGH);
  delay(10);
  ok &= expSetOutput(EXP_PIN_SD_EN, HIGH);
  delay(100);
  ok &= expSetOutput(EXP_PIN_AUDIO_PA_EN, LOW);     // amp off until audio exists
  delay(10);
  ok &= expSetOutput(EXP_PIN_LORA_RST, HIGH);

#ifdef DISPLAY_CLASS
  setDisplayPower(true);
#else
  ok &= expSetOutput(EXP_PIN_LCD_RST, LOW);
  ok &= expSetOutput(EXP_PIN_LCD_EN, LOW);
#endif

  ok &= expSetOutput(EXP_PIN_GNSS_EN, HIGH);
  delay(10);
  ok &= expSetOutput(EXP_PIN_GNSS_VRTC_EN, HIGH);
  delay(10);
  ok &= expSetOutput(EXP_PIN_GNSS_RST, HIGH);
  delay(10);
  ok &= expWritePin(EXP_PIN_GNSS_RST, LOW);
  delay(10);
  ok &= expSetOutput(EXP_PIN_GNSS_SLEEP, HIGH);
  delay(10);
  ok &= expSetOutput(EXP_PIN_GNSS_RTC_INT, LOW);

  return ok;
}

void MeshPagerX2Board::begin() {
  // AG3335 NMEA at 115200 overflows the 256-byte default during long stalls
  Serial1.setRxBufferSize(1024);

  // keep the backlight dark until a display driver takes it over
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, LOW);

  recoverI2C(PIN_BOARD_SDA, PIN_BOARD_SCL);
  recoverI2C(ENV_PIN_SDA, ENV_PIN_SCL);

  ESP32Board::begin();  // starts Wire on PIN_BOARD_SDA/PIN_BOARD_SCL (17/18)

  pinMode(PIN_USER_BTN, INPUT);       // power button, active high, external pull
  pinMode(PIN_RETURN_BTN, INPUT_PULLUP);

  expander_ok = initExpander();
  if (!expander_ok) {
    Serial.println("ERROR: TCA6424 expander init failed");
  }

  loraReset();
}

void MeshPagerX2Board::loraReset() {
  expWritePin(EXP_PIN_LORA_RST, LOW);
  delay(10);
  expWritePin(EXP_PIN_LORA_RST, HIGH);
  delay(10);
}

void MeshPagerX2Board::setGnssPower(bool on) {
  if (on) {
    expWritePin(EXP_PIN_GNSS_EN, HIGH);
    delay(10);
    expWritePin(EXP_PIN_GNSS_RST, HIGH);
    delay(10);
    expWritePin(EXP_PIN_GNSS_RST, LOW);
  } else {
    expWritePin(EXP_PIN_GNSS_EN, LOW);   // VRTC stays on for hot starts
  }
}

void MeshPagerX2Board::setDisplayPower(bool on) {
  if (on) {
    expSetOutput(EXP_PIN_LCD_EN, HIGH);
    delay(20);
    expSetOutput(EXP_PIN_LCD_RST, LOW);
    delay(20);
    expWritePin(EXP_PIN_LCD_RST, HIGH);
    delay(120);   // ST7789P3 internal init after reset
  } else {
    digitalWrite(PIN_TFT_BL, LOW);
    expWritePin(EXP_PIN_LCD_RST, LOW);
    expWritePin(EXP_PIN_LCD_EN, LOW);
  }
}

int MeshPagerX2Board::readDpad() {
  uint8_t v;
  if (!expReadReg(TCA_REG_INPUT0, v)) return -1;
  return (~v) & X2_DPAD_MASK;   // buttons pull low when pressed
}

uint16_t MeshPagerX2Board::getBattMilliVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < 4; i++) {
    mv += analogReadMilliVolts(PIN_VBAT_READ);
  }
  return (uint16_t)((mv / 4) * ADC_MULTIPLIER);
}

bool MeshPagerX2Board::isExternalPowered() {
  return expReadPin(EXP_PIN_USB_IN) == 1;
}

void MeshPagerX2Board::powerOff() {
  // logic lines low first so they can't back-power a chip losing its rail
  expWritePin(EXP_PIN_LCD_RST, LOW);
  expWritePin(EXP_PIN_GNSS_SLEEP, LOW);
  expWritePin(EXP_PIN_LCD_EN, LOW);
  expWritePin(EXP_PIN_GNSS_EN, LOW);
  expWritePin(EXP_PIN_SD_EN, LOW);
  expWritePin(EXP_PIN_AUDIO_PA_EN, LOW);
  expWritePin(EXP_PIN_SEN_EN, LOW);
  expWritePin(EXP_PIN_BAT_ADC_EN, LOW);

  // on battery, dropping the latch cuts power; on USB we fall through to
  // deep sleep instead
  expWritePin(EXP_PIN_PWR_HOLD, LOW);
  delay(50);

  ESP32Board::powerOff();
}
