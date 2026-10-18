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
# Do not seed again when testing recovery after a power interruption.
