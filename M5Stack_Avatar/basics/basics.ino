#include <M5Unified.h>
#include <Avatar.h>

#include "lgfx/v1/panel/Panel_ST7735.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"

using namespace m5avatar;

Avatar avatar;

enum panel_t { panel_unknown,
               panel_ST7735,
               panel_ST7735S,
               panel_ST7789
};

// ESP32-WROOM-32D で使用するLCDを設定.
panel_t use_panel = panel_t::panel_ST7735S;

//----------------------------------------------------------------------------
struct Panel_WROOM_ST7735S : public lgfx::Panel_ST7735S {
  Panel_WROOM_ST7735S(void) {
    _cfg.invert = true;
    _cfg.pin_cs = GPIO_NUM_5;
    _cfg.pin_rst = GPIO_NUM_18;
    _cfg.panel_width = 80;
    _cfg.panel_height = 160;
    _cfg.offset_x = 26;
    _cfg.offset_y = 1;
  }

protected:

  const std::uint8_t* getInitCommands(std::uint8_t listno) const override {
    static constexpr std::uint8_t list[] = {
      CMD_GAMMASET, 1, 0x08,  // Gamma set, curve 4
      0xFF, 0xFF,             // end
    };
    if (listno == 2) return list;
    return Panel_ST7735S::getInitCommands(listno);
  }
};

struct Panel_WROOM_ST7789 : public lgfx::Panel_ST7789 {
  Panel_WROOM_ST7789(void) {
    _cfg.invert = false;
    _cfg.pin_cs = GPIO_NUM_5;
    _cfg.pin_rst = GPIO_NUM_18;
    _cfg.panel_width = 240;
    _cfg.panel_height = 320;
    _cfg.offset_x = 0;
    _cfg.offset_y = 0;
  }
};

struct Panel_WROOM_ST7735 : public lgfx::Panel_ST7735S {
  Panel_WROOM_ST7735(void) {
    _cfg.invert = false;
    _cfg.pin_cs = GPIO_NUM_5;
    _cfg.pin_rst = GPIO_NUM_18;
    _cfg.panel_width = 128;
    _cfg.panel_height = 160;
    _cfg.memory_width = _cfg.panel_width;
    _cfg.memory_height = _cfg.panel_height;
    _cfg.offset_x = 0;
    _cfg.offset_y = 0;
  }
};

//----------------------------------------------------------------------------
void autodetect(panel_t panel) {
  auto bus_spi = new lgfx::Bus_SPI();
  auto bus_cfg = bus_spi->config();
  bus_cfg.spi_mode = 0;
  bus_cfg.use_lock = true;
  bus_cfg.spi_host = VSPI_HOST;
  bus_cfg.dma_channel = 1;

  bus_cfg.pin_mosi = GPIO_NUM_15;
  bus_cfg.pin_miso = GPIO_NUM_14;
  bus_cfg.pin_sclk = GPIO_NUM_13;
  bus_cfg.pin_dc = GPIO_NUM_23;
  bus_cfg.spi_3wire = true;
  bus_spi->config(bus_cfg);
  bus_spi->init();

  if (panel == panel_t::panel_ST7735S) {
    //  check panel (ST7735S)
    bus_cfg.freq_write = 27000000;
    bus_cfg.freq_read = 14000000;
    bus_spi->config(bus_cfg);
    auto p = new Panel_WROOM_ST7735S();
    p->bus(bus_spi);
    M5.Lcd.init(p);
  } else if (panel == panel_t::panel_ST7789) {
    //  check panel (ST7789)
    bus_cfg.freq_write = 40000000;
    bus_cfg.freq_read = 15000000;
    bus_spi->config(bus_cfg);
    auto p = new Panel_WROOM_ST7789();
    p->bus(bus_spi);
    M5.Lcd.init(p);
  } else if (panel == panel_t::panel_ST7735) {
    //  check panel (ST7735)
    bus_cfg.freq_write = 27000000;
    bus_cfg.freq_read = 14000000;
    bus_spi->config(bus_cfg);
    auto p = new Panel_WROOM_ST7735();
    p->bus(bus_spi);
    M5.Lcd.init(p);
  }
}

void update(void) {
  auto ms = m5gfx::millis();
  M5.BtnA.setRawState(ms, !digitalRead(GPIO_NUM_25));
  M5.BtnB.setRawState(ms, !digitalRead(GPIO_NUM_26));
  M5.BtnC.setRawState(ms, !digitalRead(GPIO_NUM_27));
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println(F("start"));

  M5.begin();
  Serial.println(F("M5.begin"));
  autodetect(use_panel);

  float scale = 1.0f;
  int8_t position_x = 0;
  int8_t position_y = 0;
  uint8_t display_rotation = 0;
  switch (use_panel) {
    case panel_t::panel_ST7735S:
      display_rotation = 1;
      scale = 0.6f;
      position_x = -30;
      position_y = -15;
      break;
    case panel_t::panel_ST7789:
      display_rotation = 1;
      break;
    case panel_t::panel_ST7735:
      display_rotation = 3;
      scale = 0.5f;
      break;
    case panel_t::panel_unknown:
      break;
  }
  M5.Lcd.setRotation(display_rotation);

  // avatar
  avatar.setScale(scale);
  avatar.setPosition(position_x, position_y);
  avatar.init();  // start drawing

  // button
  pinMode(GPIO_NUM_25, INPUT_PULLUP);
  pinMode(GPIO_NUM_26, INPUT_PULLUP);
  pinMode(GPIO_NUM_27, INPUT_PULLUP);
  update();

  Serial.println(F("end"));
}

void loop() {
  // avatar's face updates in another thread
  // so no need to loop-by-loop rendering
  delay(10);
  update();
  if (M5.BtnA.wasClicked()) {
    Serial.println(F("BtnA.wasClicked"));
    avatar.setExpression(Expression::Happy);
  } else if (M5.BtnB.wasClicked()) {
    Serial.println(F("BtnB.wasClicked"));
    avatar.setExpression(Expression::Sleepy);
  } else if (M5.BtnC.wasClicked()) {
    Serial.println(F("BtnC.wasClicked"));
    avatar.setExpression(Expression::Neutral);
  }
}
