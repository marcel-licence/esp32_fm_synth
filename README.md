# esp32_fm_synth
ESP32 FM Synthesizer MIDI Module, arduino project
(sounds like YM2612, Sega Genesis/Mega Drive)

- video presentation of the initial state of this project https://youtu.be/rGTw05GKwvU
- little quick start guide to get started with arduino synthesizer / music projects: https://youtu.be/ZNxGCB-d68g

---
## Overview

This project is a MIDI FM synthesizer module. The sound will be generated completely by the ESP32.
It behaves like an emulated YM1612 (but not completely exactly like it)
The FM core works with float precision.

- 8 algorithm implemented (same as YM2612)
- max polyphony of 6 voices at same time (each voice with 4 operators)
- each of 16 MIDI channels have own sound (different setting)
- envelope (attack, decay1, sustain, decay2, release)
- fine and coarse tuning of each operator
- additional dynamic by velocity of played notes to total level of an operator
- pitch bending and modulation
- reverb effect
- delay/echo effect
- dump of current voice setting (prints out code which can be put into c file)
- turn off quietest notes when polyphony exceeded to allow playing new notes

---
## Usage

This project is not for commercial use (please contact me if you have commercial requests)
The purpose of this project is to get into FM synthesier, programming and having just fun with it.

I prepared presets on all channels for easier access.

---
## Bugs or problems

You can enter found issues here: https://github.com/marcel-licence/esp32_fm_synth/issues
Please add only issues found when using the DOIT ESP32 DEVKIT with the original configuration

If you are try using other platforms, other configurations please use the discussion area:
https://github.com/marcel-licence/esp32_fm_synth/discussions

---
## Compiling

This project can be compiled with arduino 1.8.13.
Using board 'esp32' from platform in folder: ...\esp32\1.0.4
Using core 'esp32' from platform in folder: ...\esp32\1.0.4

Version of my used libraries are:
  FS.h [FS@1.0]
  LITTLEFS.h [LittleFS_esp32@1.0.5]
  SD_MMC.h [SD_MMC@1.0]
  WiFi.h [WiFi@1.2.7 WiFi@1.0]
  AC101.h [AC101@0.0.1]
  Wire.h [Wire@1.0.1]

PSRAM must be disabled

---
## Required hardware

This project can be used with the DOIT ESP32 DEVKIT using an external DAC based on PCM5201 or the ESP32 Audio Kit V2.2 using the onboard AC101 DAC (please enable: #define ESP32_AUDIO_KIT in config.h)
Please find hardware connections defined in config.h
(additional information will follow soon)

---
## Project structure

You will find defines and configuration in config.h as well z_config.ino.
Pins can be changed and also the MIDI mapping can be tweaked for your controller.

esp32_fm_synth.ino is the main project file combining all the modules.

The other files in the project are for single modules named by its purpose.

---
## MIDI connection

MIDI can be used by using an opto-coupler to get a serial signal to connect to RXD2 (defined in config.h).
You can also feed in a MIDI signal via the serial via USB. When MIDI_RECV_FROM_SERIAL is set the device will listen to its serial port for MIDI messages.
Please let me know if you find a tool I can reference.

