#pragma once

// DW3000 SPI
#define BOARD_UWB_SPI_SCLK_GPIO      18
#define BOARD_UWB_SPI_MISO_GPIO      19
#define BOARD_UWB_SPI_MOSI_GPIO      23
#define BOARD_UWB_SPI_CS_GPIO        4

// DW3000 control
#define BOARD_UWB_IRQ_GPIO           34  // input-only on ESP32
#define BOARD_UWB_RST_GPIO           27
#define BOARD_UWB_WAKEUP_GPIO        32
#define BOARD_UWB_EXTON_GPIO         33

// Optional board peripherals
#define BOARD_STATUS_LED_GPIO        2
