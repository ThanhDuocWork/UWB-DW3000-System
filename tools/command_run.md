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
