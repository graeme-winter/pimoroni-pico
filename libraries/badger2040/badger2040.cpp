#include <string.h>
#include <math.h>

#include "hardware/pwm.h"
#include "hardware/watchdog.h"

#include "badger2040.hpp"

namespace pimoroni {

  void Badger2040::init() {
    // set clock speed to 12MHz to reduce the maximum current draw on the
    // battery. when updating a small, monochrome, display only every few
    // seconds or so then you don't need much processing power anyway...
    //set_sys_clock_khz(48000, true);

    gpio_set_function(ENABLE_3V3, GPIO_FUNC_SIO);
    gpio_set_dir(ENABLE_3V3, GPIO_OUT);
    gpio_put(ENABLE_3V3, 1);

    gpio_set_function(A, GPIO_FUNC_SIO);
    gpio_set_dir(A, GPIO_IN);
    gpio_set_pulls(A, false, true);

    gpio_set_function(B, GPIO_FUNC_SIO);
    gpio_set_dir(B, GPIO_IN);
    gpio_set_pulls(B, false, true);

    gpio_set_function(C, GPIO_FUNC_SIO);
    gpio_set_dir(C, GPIO_IN);
    gpio_set_pulls(C, false, true);

    gpio_set_function(D, GPIO_FUNC_SIO);
    gpio_set_dir(D, GPIO_IN);
    gpio_set_pulls(D, false, true);

    gpio_set_function(E, GPIO_FUNC_SIO);
    gpio_set_dir(E, GPIO_IN);
    gpio_set_pulls(E, false, true);

    gpio_set_function(USER, GPIO_FUNC_SIO);
    gpio_set_dir(USER, GPIO_IN);
    gpio_set_pulls(USER, false, true);

    gpio_set_function(VBUS_DETECT, GPIO_FUNC_SIO);
    gpio_set_dir(VBUS_DETECT, GPIO_IN);
    gpio_put(VBUS_DETECT, 1);

    // read initial button states
    uint32_t mask = (1UL << A) | (1UL << B) | (1UL << C) | (1UL << D) | (1UL << E);
    _wake_button_states |= gpio_get_all() & mask;

/*
    // wait for button to be released before continuing
    while(gpio_get_all() & mask) {
      tight_loop_contents();
    }
*/

    // led control pin
    pwm_config cfg = pwm_get_default_config();
    pwm_set_wrap(pwm_gpio_to_slice_num(LED), 65535);
    pwm_init(pwm_gpio_to_slice_num(LED), &cfg, true);
    gpio_set_function(LED, GPIO_FUNC_PWM);
    led(0);

    uc8151.init();

    // TODO: set default image?
  }

  void Badger2040::clear() {
    for(uint32_t y = 0; y < 128; y++) {
      for(uint32_t x = 0; x < 296; x++) {
        pixel(x, y);
      }
    }
  }

  void Badger2040::halt() {
    gpio_put(ENABLE_3V3, 0);

    // If running on USB we will not actually power down, so emulate the behaviour
    // of battery powered badge by listening for a button press and then resetting
    // Note: Don't use wait_for_press as that waits for the button to be release and
    //       we want the reboot to complete before the button is released.
    update_button_states();
    while(_button_states == 0) {
      update_button_states();
    }
    watchdog_reboot(0, 0, 0);
  }

  uint8_t _dither_value(int32_t x, int32_t y, uint8_t p) {
    // ordered dither matrix used in 4-bit mode
    static uint8_t _odm[16] = {
      0,  8,  2, 10,
      12,  4, 14,  6,
      3, 11,  1,  9,
      15,  7, 13,  5
    };

    // calculate dither matrix offset
    uint32_t dmo = (x & 0b11) | ((y & 0b11) << 2);

    if(p == 0) {
      return 1;
    }else if(p == 15) {
      return 0;
    }else{
      return p <= _odm[dmo] ? 1 : 0;
    }
  }

  void Badger2040::pixel(int32_t x, int32_t y) {
    if(_thickness == 1) {
      uc8151.pixel(x, y, _dither_value(x, y, _pen));
    }else{
      uint8_t ht = _thickness / 2;
      for(int sy = 0; sy < _thickness; sy++) {
        for(int sx = 0; sx < _thickness; sx++) {
          uc8151.pixel(x + sx - ht, y + sy - ht, _dither_value(x + sx - ht, y + sy - ht, _pen));
        }
      }
    }
  }

  // Display a portion of an image (icon sheet) at dx, dy
  void Badger2040::icon(const uint8_t *data, int sheet_width, int icon_size, int index, int dx, int dy) {
    image(data, sheet_width, icon_size * index, 0, icon_size, icon_size, dx, dy);
  }

  // Display an image that fills the screen (286*128)
  void Badger2040::image(const uint8_t* data) {
    image(data, 296, 0, 0, 296, 128, 0, 0);
  }

  // Display an image smaller than the screen (sw*sh) at dx, dy
  void Badger2040::image(const uint8_t *data, int w, int h, int x, int y) {
    image(data, w, 0, 0, w, h, x, y);
  }

  void Badger2040::image(const uint8_t *data, int stride, int sx, int sy, int dw, int dh, int dx, int dy) {
    for(auto y = 0; y < dh; y++) {
      for(auto x = 0; x < dw; x++) {
        // work out byte offset in source data
        uint32_t o = ((y + sy) * (stride >> 3)) + ((x + sx) >> 3);

        // extract bitmask for this pixel
        uint32_t bm = 0b10000000 >> ((x + sx) & 0b111);

        // draw the pixel
        uc8151.pixel(dx + x, dy + y, data[o] & bm);
      }
    }
  }

  void Badger2040::rectangle(int32_t x, int32_t y, int32_t w, int32_t h) {
    for(int cy = y; cy < y + h; cy++) {
      for(int cx = x; cx < x + w; cx++) {
        pixel(cx, cy);
      }
    }
  }

  void Badger2040::line(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t	x = x1, y = y1, dx, dy, incx, incy, balance;

    if(x2 >= x1) {dx = x2 - x1; incx = 1;} else {dx = x1 - x2; incx = -1;}
    if(y2 >= y1) {dy = y2 - y1; incy = 1;} else {dy = y1 - y2; incy = -1;}

    if(dx >= dy) {
      dy <<= 1; balance = dy - dx; dx <<= 1;
      while(x != x2) {
        pixel(x, y);
        if (balance >= 0) {y += incy; balance -= dx;}
        balance += dy; x += incx;
      }
    } else {
      dx <<= 1; balance = dx - dy; dy <<= 1;
      while(y != y2) {
        pixel(x, y);
        if(balance >= 0) {x += incx; balance -= dy;}
        balance += dx; y += incy;
      }
    }
  }

  void Badger2040::debug_command(uint8_t reg, size_t len, const uint8_t *data) {
    uc8151.command(reg, len, data);
  }

  void Badger2040::dump_otp(uint8_t *data) {
    uc8151.read(0xa2,  0xFFF, data);
  }

  void Badger2040::update_button_states() {
    uint32_t mask = (1UL << A) | (1UL << B) | (1UL << C) | (1UL << D) | (1UL << E);
    _button_states = gpio_get_all() & mask;
  }

  uint32_t Badger2040::button_states() {
    return _button_states;
  }

  bool Badger2040::is_busy() {
    return uc8151.is_busy();
  }

  void Badger2040::power_off() {
    uc8151.power_off();
  }

  void Badger2040::invert(bool invert) {
    uc8151.invert(invert);
  }

  void Badger2040::update_speed(uint8_t speed) {
    uc8151.update_speed(speed);
  }

  void Badger2040::partial_update(int x, int y, int w, int h, bool blocking) {
    uc8151.partial_update(x, y, w, h, blocking);
  }

  void Badger2040::update(bool blocking) {
    uc8151.update(blocking);
  }

  const hershey_font_glyph_t* Badger2040::glyph_data(unsigned char c) {
    if(c < 32 || c > 127) {
      return nullptr;
    }

    return &_font->chars[c - 32];
  }

  inline float deg2rad(float degrees) {
    return (degrees * M_PI) / 180.0f;
  }

  int32_t Badger2040::glyph(unsigned char c, int32_t x, int32_t y, float s, float a) {
    const hershey_font_glyph_t *gd = glyph_data(c);

    // if glyph data not found (id too great) then skip
    if(!gd) {
      return 0;
    }

    a = deg2rad(a);
    float as = sin(a);
    float ac = cos(a);

    const int8_t *pv = gd->vertices;
    int8_t cx = (*pv++) * s;
    int8_t cy = (*pv++) * s;
    bool pen_down = true;

    for(uint32_t i = 1; i < gd->vertex_count; i++) {
      if(pv[0] == -128 && pv[1] == -128) {
        pen_down = false;
        pv += 2;
      }else{
        int8_t nx = (*pv++) * s;
        int8_t ny = (*pv++) * s;

        int rcx = cx * ac - cy * as;
        int rcy = cx * as + cy * ac;

        int rnx = nx * ac - ny * as;
        int rny = nx * as + ny * ac;

        if(pen_down) {
          line(rcx + x, rcy + y, rnx + x, rny + y);
        }

        cx = nx;
        cy = ny;
        pen_down = true;
      }
    }

    return gd->width * s;
  }

  void Badger2040::text(std::string message, int32_t x, int32_t y, float s, float a) {
    int32_t cx = x;
    int32_t cy = y;

    int32_t ox = 0;

    float as = sin(deg2rad(a));
    float ac = cos(deg2rad(a));

    for(auto &c : message) {
      int rcx = ox * ac;
      int rcy = ox * as;

      ox += glyph(c, cx + rcx, cy + rcy, s, a);
    }
  }

  int32_t Badger2040::measure_text(std::string message, float s) {
    int32_t width = 0;
    for(auto &c : message) {
      width += measure_glyph(c, s);
    }
    return width;
  }

  int32_t Badger2040::measure_glyph(unsigned char c, float s) {
    const hershey_font_glyph_t *gd = glyph_data(c);

    // if glyph data not found (id too great) then skip
    if(!gd) {
      return 0;
    }

    return gd->width * s;
  }


  void Badger2040::font(std::string name) {
    // check that font exists and assign it
    if(fonts.find(name) != fonts.end()) {
      _font = fonts[name];
    }
  }

  void Badger2040::pen(uint8_t pen) {
    _pen = pen;
  }

  void Badger2040::thickness(uint8_t thickness) {
    _thickness = thickness;
  }

  void Badger2040::led(uint8_t brightness) {
    // set the led brightness from 1 to 256 with gamma correction
    float gamma = 2.8;
    uint16_t v = (uint16_t)(pow((float)(brightness) / 256.0f, gamma) * 65535.0f + 0.5f);
    pwm_set_gpio_level(LED, v);
  }

  bool Badger2040::pressed(uint8_t button) {
    return (_button_states & (1UL << button)) != 0;
  }

  bool Badger2040::pressed_to_wake(uint8_t button) {
    return (_wake_button_states & (1UL << button)) != 0;
  }

  void Badger2040::wait_for_press() {
    update_button_states();
    while(_button_states == 0) {
      update_button_states();
      tight_loop_contents();
    }

    uint32_t mask = (1UL << A) | (1UL << B) | (1UL << C) | (1UL << D) | (1UL << E);
    while(gpio_get_all() & mask) {
      tight_loop_contents();
    }
  }
}
