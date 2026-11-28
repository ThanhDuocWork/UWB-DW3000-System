# WeAct LA to Makerfabs ESP32-UWB-DW3000 SW: pulseview

```text
WeAct LA                  ESP32 / DW3000
--------                  --------------
GND      ---------------- GND

CH0      ---------------- CS     GPIO4
CH1      ---------------- SCLK   GPIO18
CH2      ---------------- MOSI   GPIO23
CH3      ---------------- MISO   GPIO19

CH4      ---------------- RST    GPIO27   
CH5      ---------------- IRQ    GPIO34   
CH6      ---------------- WAKEUP GPIO32   
```

Power the kit via USB. Leave the analyzer's power pin disconnected.

## PulseView preset

```text
Device:        sigrok FX2 (fx2lafw)
Sample count:  10M samples
Sample rate:   8 MHz (analyzer clock)
SPI decoder:   CS#=CH0, CLK=CH1, MOSI=CH2, MISO=CH3
SPI settings:  Mode 0, MSB first, 8 bits, CS active low
```
