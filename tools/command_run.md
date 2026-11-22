## ESP-IDF environment
& 'C:\Espressif\tools\Microsoft.v5.5.3.PowerShell_profile.ps1'

## Anchor
.\tools\build_anchor.ps1
idf.py -B build_anchor -p COM3 flash
idf.py -B build_anchor -p COM3 monitor
idf.py -B build_anchor -p COM3 flash monitor

## Tag
.\tools\build_tag.ps1
idf.py -B build_tag -p COM5 flash
idf.py -B build_tag -p COM5 monitor
idf.py -B build_tag -p COM5 flash monitor

## Boot swap: dummy-data ESP32 flash lab (replaces UWB app on selected board)
# Save working UWB artifacts first. See OTA/flash_lab/README.md.
.\tools\build_swap_lab.ps1
$labBuild = Join-Path $PWD 'testing\boot_swap_esp32'
idf.py -C OTA/flash_lab -B $labBuild -p COM3 flash monitor
# At swap-lab>: seed ERASE, swap, status, confirm, reset
# Deterministic cut: from healthy Idle run swap_pause. No preliminary reboot.
# Wait for TEST PAUSED: scratch partially written; cut power now (256/4096).
# Console is frozen; physically cut power. RAM pause flag clears on reset.
# Do NOT seed after cut: reconnect, expect Trial; confirm or reset to roll back.
# Do not seed again when testing recovery after a power interruption.

## TC09: journal pause (commands at swap-lab>, no seed needed)
# Baseline: status -> Idle primary=A secondary=B boot_ok=1.
status
journal_pause request
# Wait: TEST PAUSED: journal body written before seal; cut power now
# Evidence: sector=7/8 offset=0 written=28/32 seal=0xffffffff (readback).
# Frozen console: remove ALL power/backpower (USB/debugger/GPIO), then restore.
# No seed. Expect Idle A/B boot_ok=1.
status
journal_pause checkpoint
# Wait for same marker/evidence; remove all power and restore, no seed.
# Expect Trial B/A boot_ok=1. Do not confirm. Then:
reset
status
# Expect Idle A/B boot_ok=1. For baseline B/A, reverse all expected letters.
# Monitor reconnect may add resets: save reset reasons and FIRST startup status.
# Extra reset after Trial rolls back. EN is reset only, not a power-cut result.
# RAM-only hook; between body and seal writes, NOT a torn-write test.
# No hardware PASS implied; host CTest name: boot_swap_journal_pause.
