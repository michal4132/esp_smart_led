# ESP8266 / ESP32 Smart LED

Simple Smart LED without need for external services.

# Configuration

Set pin numbers for each LED in led_task.h

Set WiFi credentials in config.json and upload to ESP.

# Building

Clone repository and init submodules

`git submodule update --init --recursive`

Build with idf.py

`idf.py build`

Flash

`idf.py flash`

Upload files from www directory by launching http://[esp ip]/upload.

Firmware can be upgraded at /upgrade

# TODO

-ESP32 support
