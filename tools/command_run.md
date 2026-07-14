## Anchor
.\tools\use_anchor_config.ps1
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
## Tag
.\tools\use_tag_config.ps1
idf.py build
idf.py -p COM5 flash
idf.py -p COM5 monitor

& 'C:\Espressif\tools\Microsoft.v5.5.3.PowerShell_profile.ps1'