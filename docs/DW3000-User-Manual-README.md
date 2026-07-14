# DW3000 User Manual Notes

Reference document in this folder:
- [DW3000-User-Manual.pdf](./DW3000-User-Manual.pdf)

This file is a practical reading guide for the `DW3000 User Manual` from a firmware bring-up and driver development perspective. The goal is to help identify which sections to read first, especially when working on:
- `SPI bring-up`
- `reset / wakeup / DEV_ID`
- `TX/RX frame flow`
- `status / IRQ`
- `register map`
- `fast commands`
- `two-way ranging`

## 1. Overall document structure

| Section | Content | Importance for this project |
| --- | --- | --- |
| 1 | DW3000 family introduction | Medium |
| 2 | Architecture, SPI, state machine, PHY | Very important |
| 3 | Frame transmission flow | Very important |
| 4 | Frame reception flow | Very important |
| 5 | Hardware MAC features | Important |
| 6 | Secure ranging, STS | Learn later |
| 7 | OTP, GPIO, temperature sensing, external sync | Learn later |
| 8 | Complete register map | Most important |
| 9 | Fast Commands | Very important |
| 10 | Calibration | Important when accuracy matters |
| 11 | Positioning models | System overview |
| 12 | Two-Way Ranging | Important for ranging work |

---

## 2. Section 1: DW3000 family introduction

This section gives the high-level overview:
- what the DW3000 is
- main operating modes
- related product family context
- where the chip fits in a UWB positioning system

### What to get from it
- understand the scope of the chip
- know what classes of applications it supports
- understand that DW3000 sits mainly at the PHY/timestamping/frame-exchange level

### What is not critical yet
- marketing-style overview material
- this is not where register addresses should be looked up

---

## 3. Section 2: Architecture, SPI, state machine, PHY

This is one of the most important sections during board bring-up.

### Read this carefully for
- the chip operating model
- `idle / rx / tx / sleep / wakeup` states
- the `SPI` interface
- how the chip accepts commands and register access
- core PHY concepts: channel, preamble, SFD, data rate, PAC

### Practical goal
From this section, you should be able to understand the flow:

```text
ESP32 init SPI
  -> reset DW3000
  -> wake up if needed
  -> read DEV_ID
  -> verify the chip is alive
  -> continue with SYS_STATUS
```

### This is where to look up
- SPI read/write header format
- register file and offset model
- register addressing over SPI
- the chip state machine basics

### When debugging
If `DEV_ID` cannot be read correctly, come back to this section first.

---

## 4. Section 3: Frame transmission flow

This is very important for `TX` bring-up.

### Read this to understand
- how data is written into the `TX buffer`
- what `TX frame control` configures
- when the chip starts transmitting
- which status bit indicates `TX done`
- what TX-related failures exist

### Items to look up in the manual
- `TX buffer`
- `TX_FCTRL`
- `SYS_STATUS` TX bits
- `Fast command` used to start TX

### Mapping to the current code
These concepts map to:
- `TX_BUFFER`
- `TX_FCTRL`
- `CMD_TX`
- `SYS_STATUS_TXFRS`

### Important note
`TX_FCTRL` does not only contain frame length. It also contains PHY/TX mode-related fields. If code overwrites the whole register during TX, it can accidentally destroy radio configuration that was applied earlier.

---

## 5. Section 4: Frame reception flow

This is very important for `RX` bring-up.

### Read this to understand
- how the chip enters RX mode
- how a frame is detected and validated
- what indicates a successful RX event
- which bits report RX errors
- where to read received data from

### Items to look up in the manual
- `RX enable`
- `RX good frame`
- `RX timeout`
- `PHY/header/frame errors`
- `RX_FINFO`
- `RX_BUFFER_0`
- `SYS_STATUS` RX bits

### Mapping to the current code
These concepts map to:
- `CMD_RX`
- `SYS_STATUS_RXFCG`
- `SYS_STATUS_ALL_RX_ERR`
- `RX_FINFO`
- `RX_BUFFER_0`

### Important note
If the transmitter reports `TX done` but the receiver never reports `RX good frame`, the problem is usually one of these:
- channel mismatch
- preamble/code/SFD mismatch
- incorrect timeout
- RX status bits not cleared correctly

---

## 6. Section 5: Hardware MAC features

This section is important, but not the first one to study when only bringing up a basic two-board link.

### Read this to learn
- which MAC-level features are implemented in hardware
- whether filtering, acknowledgements, or addressing support exist
- which parts can be offloaded to hardware versus firmware

### Why it matters
- helps decide what should remain in firmware
- helps identify what can be delegated to the chip

### In this project
The current project uses a custom lightweight frame format at the application/MAC level, so this section is more architectural guidance than an immediate implementation requirement.

---

## 7. Section 6: Secure ranging, STS

This section should be studied later, after a normal link and normal ranging are already working.

### Read this to learn
- what STS is
- how secure ranging works
- how it affects PHY and frame exchange

### For the current stage
- not a first priority
- only necessary once the basic radio and ranging path are stable

---

## 8. Section 7: OTP, GPIO, temperature sensing, external sync

This section becomes useful when the product starts needing deeper hardware integration and calibration handling.

### It includes
- OTP memory
- auxiliary GPIO usage
- internal temperature or voltage sensing
- external synchronization support

### When to read it
- when calibration values should be stored in OTP
- when DW3000 GPIO features are needed
- when multiple anchors or synchronized nodes are used

### At the current stage
- not a first-priority section for basic TX/RX bring-up

---

## 9. Section 8: Complete register map

This is the most important section in the entire manual for driver work.

### This is where to look up
- register names
- register addresses
- register file and sub-offset organization
- register length
- bit fields
- reset/default values
- access type such as `RO / RW / W1C`

### The first registers to learn
- `DEV_ID`
- `SYS_STATUS`
- `TX_FCTRL`
- `CHAN_CTRL`
- `RX_FINFO`
- `DTUNE0`
- `RX_BUFFER_0`
- `TX_BUFFER`

### Very important
When a constant in code looks like:
- `DEV_ID = 0x000000`
- `SYS_STATUS = 0x000044`
- `CHAN_CTRL = 0x010014`

its correct source is the `register map` section of the manual, not a community repository.

### Correct lookup method
For each register, always identify these five things:
1. address / file ID / offset
2. length
3. bit field definition
4. read/write behavior
5. how status bits are cleared if it is a status register

`SYS_STATUS` is especially important because many bits must be cleared explicitly and correctly.

---

## 10. Section 9: Fast Commands

This is a very important section for real firmware work.

### Read this to understand
- which chip actions can be triggered directly without a long register sequence
- when fast commands should be used
- how fast commands affect the state machine

### Typical commands
- `TX`
- `RX`
- `TXRXOFF`

### Why it matters
- simplifies control flow
- reduces command latency
- matches how practical DW3000 driver code is usually structured

### Mapping to the current project
These concepts map to:
- `CMD_TX`
- `CMD_RX`
- `CMD_TXRXOFF`

---

## 11. Section 10: Calibration

This section becomes important when stable and accurate measurements are required.

### It includes
- antenna delay
- timing or RF-related tuning
- calibration values that must be measured or stored

### When to read it
- after basic TX/RX is already working
- after basic ranging is running
- when distance error needs to be reduced

### At the current stage
- not the first priority
- but it will become important once `TWR` starts working

---

## 12. Section 11: Positioning models

This section is more about system architecture than register bring-up.

### Read it to understand
- the roles of anchor and tag
- how ranging leads to positioning
- how multi-anchor systems are organized

### Why it matters
- helps frame the system correctly
- explains where the chip fits in the larger solution

### Important note
This section does not replace the need to understand the register map or state machine when debugging the driver.

---

## 13. Section 12: Two-Way Ranging

This section is very important when moving from a `PING link test` to actual distance measurement.

### Read this to understand
- `poll / response / final`
- where timestamps are captured
- how distance is calculated
- timeout and sequencing between nodes

### When to study it
- after:
  - `DEV_ID` can be read correctly
  - TX/RX works between two boards
  - `SYS_STATUS`, `RX_FINFO`, and `TX_FCTRL` are already understood

### Why it matters
This section connects the low-level driver to the ranging application layer:
- the driver handles register access, IRQ, TX, RX
- TWR logic handles the ranging protocol itself

---

## 14. Recommended reading order for this project

### Stage 1: SPI bring-up and basic chip verification
1. Section 2
2. Section 8
3. Section 9

### Stage 2: Frame transmission and reception
1. Section 3
2. Section 4
3. Section 8
4. Section 9

### Stage 3: Correct radio configuration
1. Section 2
2. Section 8
3. Section 10

### Stage 4: Ranging
1. Section 12
2. Section 10
3. Section 11

---

## 15. Registers to look up first when debugging

### Minimal bring-up
- `DEV_ID`
- `SYS_STATUS`

### TX path
- `TX_BUFFER`
- `TX_FCTRL`
- `SYS_STATUS`

### RX path
- `SYS_STATUS`
- `RX_FINFO`
- `RX_BUFFER_0`

### Radio configuration
- `CHAN_CTRL`
- `TX_FCTRL`
- `DTUNE0`

---

## 16. Very short summary

If the manual is being read for practical firmware work, prioritize it in this order:

```text
1. SPI / state machine
2. Register map
3. Fast commands
4. TX flow
5. RX flow
6. TWR
```

If only one idea should be remembered, it is this:

```text
The most important parts of the DW3000 User Manual for driver work are:
- SPI/register addressing
- register map
- SYS_STATUS / TX_FCTRL / CHAN_CTRL / RX_FINFO / buffer registers
- fast commands
```
