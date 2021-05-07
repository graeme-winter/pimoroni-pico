#pragma once

#include "../../drivers/st7735/st7735.hpp"
#include "../../libraries/pico_graphics/pico_graphics.hpp"

namespace pimoroni {

  class BreakoutColourLCD160x80 : public PicoGraphics {
    //--------------------------------------------------
    // Constants
    //--------------------------------------------------
  public:
    static const int WIDTH            = 160;
    static const int HEIGHT           = 80;
    static const uint8_t PIN_UNUSED   = UINT8_MAX;


    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
  public:
    uint16_t *__fb;
  private:
    ST7735 screen;


    //--------------------------------------------------
    // Constructors/Destructor
    //--------------------------------------------------
  public:
    BreakoutColourLCD160x80(uint16_t *buf);
    BreakoutColourLCD160x80(uint16_t *buf, spi_inst_t *spi,
      uint8_t cs, uint8_t dc, uint8_t sck, uint8_t mosi, uint8_t miso = PIN_UNUSED, uint8_t bl = PIN_UNUSED);
    BreakoutColourLCD160x80(uint16_t *buf, ST7735::BG_SPI_SLOT slot);


    //--------------------------------------------------
    // Methods
    //--------------------------------------------------
  public:
    void init();

    spi_inst_t* get_spi() const;
    int get_cs() const;
    int get_dc() const;
    int get_sck() const;
    int get_mosi() const;
    int get_bl() const;

    void update();
    void set_backlight(uint8_t brightness);
  };

}
