# dlshook
Drum Live Station Hook Library

dlshook is library to be used with LD_PRELOAD to enable playing drum live station

# Building
## Installing Necessary Dependencies
TODO

## Compiling with cmake

```console
cmake -B build && cmake --build build
```

# Configuration

dlshook can be configured to some exten using environment variables.

|variable|default value if not present|description|
|--------|----------------------------|-----------|
|DLS_KEYS_PATH|dls.keys|specify the path to keys file|
|CARD_PATH|/SAVE/DLS/card.bin|specify where card emulation data is stored|
|SCRIPTS_PATH|SCRIPT/|specify the directory to redirect all file accesses for "/SCRIPT/" folder|
|SETTINGS_PATH|/SAVE/DLS/|specify the directory to redirect all file accesses for "/SETTINGS/" folder|

# Supported Features
## Basic Dongle Emulation
dlshook has basic support to "emulate" the prescence of usb dongle

## SETTINGS and SCRIPTS Redirection
dlshook will try to redirect files from SETTINGS and SCRIPTS to a different folder

## Case Insensitive File System redirection
dls game filesystem is FAT32 meaning that filename are case insensitive, adittionally the game is quite inconsistent with filenamings.

To address this specific issue dlshook will take a snapshot of the game directory at the beginning to workaround this issue.

## Card Reader Emulation
dlshook has implemented a simple card reader emulation to enable you playing all content.

# Non Supported Features
* HDD Lock emulation, you need to use a patched version of the game with hdd lock protection disabled.
* Card Swap/removal, as for the moment game will think you always have the same card connected.
* Non NVIDIA cards fix, you need to use a patched version of the game with this fix.
* IO emulation, there're already public hooks for this so I just saved some effort.

# Special Thanks
I would like to express my special thanks to *piuitemsmx* for his big effort in testing and catching bugs promptly, delivering this hook in such a short amount of time wouldn't be possible without him.
