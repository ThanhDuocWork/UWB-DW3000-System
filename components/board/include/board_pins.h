#pragma once

namespace board {

struct Pins {
    int spi_sck = 18;
    int spi_mosi = 23;
    int spi_miso = 19;
    int uwb_cs = 4;
    int uwb_rst = 27;
    int uwb_irq = 34;
    int status_led = 2;
};

const Pins &pins();

}  // namespace board
