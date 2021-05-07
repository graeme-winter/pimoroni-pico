#pragma once

#include "../../drivers/st7789/st7789.hpp"
#include "../../libraries/pico_graphics/pico_graphics.hpp"

namespace pimoroni {

  class BreakoutRoundLCD : public PicoGraphics {
    //--------------------------------------------------
    // Constants
    //--------------------------------------------------
  public:
    static const int WIDTH = 240;
    static const int HEIGHT = 240;


    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
  public:
    uint16_t *__fb;
  private:
    ST7789 screen;


    //--------------------------------------------------
    // Constructors/Destructor
    //--------------------------------------------------
  public:
    BreakoutRoundLCD(uint16_t *buf);
    BreakoutRoundLCD(uint16_t *buf,  spi_inst_t *spi,
      uint8_t cs, uint8_t dc, uint8_t sck, uint8_t mosi, uint8_t miso = -1, uint8_t bl = -1);
    BreakoutRoundLCD(uint16_t *buf,  ST7789::BG_SPI_SLOT slot);
    

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
