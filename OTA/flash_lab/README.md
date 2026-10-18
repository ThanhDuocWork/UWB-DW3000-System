# ESP32 scratch-swap flash lab

This is a separate ESP-IDF v5.5.3 / ESP32 application. It runs the swap engine
against REAL flash but swaps only dummy A/B byte patterns. It does not execute
those patterns, modify the bootloader, run UWB, or download OTA images.

## Before flashing

- Use ONE development board. Flashing this lab replaces its UWB application.
- Save the working UWB source/config/images and a recovery procedure first.
- The supplied table preserves the existing NVS, PHY, factory and storage
  addresses, and reserves 0x3f0000..0x3f8fff for the lab on 4MB flash. Confirm
  your actual device/table has no other data there. Do not use on production.
- Do not run erase-flash. Do not enable Secure Boot, flash encryption or change
  eFuses for this experiment. The adapter refuses encrypted flash.
- The runtime erase/write adapter is bounded to the named data partition;
  the separate serial flash step still writes bootloader/table/application.

## Build and flash (from repository root)

Use the ESP-IDF terminal, NOT the MSVC terminal used for host tests:

```powershell
& 'C:\Espressif\tools\Microsoft.v5.5.3.PowerShell_profile.ps1'
.\tools\build_swap_lab.ps1
$labBuild = Join-Path $PWD 'testing\boot_swap_esp32'
idf.py -C OTA/flash_lab -B $labBuild -p COM3 flash monitor
```

Replace COM3 with the selected board's port. No second board is needed.
Close any existing monitor on that port. Firmware logs and commands use UART
115200 baud. A terminal that supports the ESP-IDF REPL is required.
Each project keeps its own sdkconfig/build directory; UWB profiles are untouched.

## Physical test regions

| Region | Address | Size |
| --- | --- | --- |
| Dummy primary | 0x3f0000 | 12 KiB (sectors 0..2) |
| Dummy secondary | 0x3f3000 | 12 KiB (sectors 3..5) |
| Scratch | 0x3f6000 | 4 KiB (sector 6) |
| Journal A | 0x3f7000 | 4 KiB (sector 7) |
| Journal B | 0x3f8000 | 4 KiB (sector 8) |

All byte patterns are checked in full, not merely a version string or first
word. '?' in status means a pattern does not match a complete known A or B.
These are not bootable firmware images and do not test image authentication.

## Commands at swap-lab>

| Command | Action |
| --- | --- |
| help | Show commands |
| status | Read journal and compare dummy patterns (no writes) |
| seed ERASE | Destroy ONLY lab contents; initialize primary A, secondary B |
| swap | Require healthy Idle/A-B pair; persist request then restart |
| confirm | Require healthy Trial/A-B pair; accept current primary |
| reset | Restart; Trial without confirm will swap back |

No automatic provisioning: blank/invalid journal stops swap and leaves the
console available. Save evidence before using seed ERASE after a failure.
Seeding is destructive and not resumable; after interruption during seeding,
repeat seed ERASE explicitly. Do not confuse this with swap recovery.

## Test 1: accept new dummy image

1. On first boot expect UNPROVISIONED_OR_INVALID.
2. Run `seed ERASE`: expect Idle, primary=A, secondary=B, boot_ok=1.
3. Run `swap`: automatic restart, then expect Trial, primary=B, secondary=A.
4. Run `confirm`: expect Idle, primary=B, secondary=A.
5. Run `reset`: expect Idle and the same B/A pair.

## Test 2: reject new dummy image

1. Run `seed ERASE`, then `swap`.
2. In Trial do NOT confirm. Run `reset` (or use EN).
3. Expect reverse swap followed by Idle, primary=A, secondary=B, boot_ok=1.

## Test 3: manual power interruption

1. Run `seed ERASE`, then `swap`.
2. During ERASE/PROGRAM logs, remove board power, then reconnect it and monitor.
3. Do NOT seed again: startup must read journal and resume the pending swap.
4. Expect a complete A/B or B/A pair, never boot_ok=1 with '?' patterns.
   Forward completion normally reaches Trial B/A. If the Trial marker was
   already committed before power loss, the next startup reverts to Idle A/B.
5. Repeat during reverse swap: final result must be Idle A/B.
6. Save the reset reason, last sector/offset, and final status for each attempt.

Writes use <=256-byte chunks with a 50ms delay after each erase/program to
expose checkpoints. Cutting during a delay is NOT proof of handling a torn
flash operation; repeat at varying times. USB power removal is a true power
cut only if no other supply/debugger/GPIO connection is back-powering the board.
The reset command/EN is a reset test, not the same as removing power.
Do not cut power while serial flashing the bootloader/partition table.

## Return to UWB

Exit monitor (Ctrl+]) and use the existing root-project build/flash commands:

```powershell
.\tools\build_anchor.ps1
idf.py -B build_anchor -p COM3 flash monitor
```

For a tag use build_tag.ps1 and build_tag instead. Full flash restores the UWB
partition table/application. Dummy lab bytes may remain in unused flash but
are no longer referenced by the original table. Do not erase NVS to clean them.

## Limits

This is an application-level flash adapter, NOT a second-stage bootloader
adapter. Primary dummy bytes are never the executing application. Hardware
power-cut results must be collected before integrating actual image boot.
No firmware build/board validation is implied by the host test result.
