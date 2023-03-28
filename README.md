# XBee_controller_STM32_ESP32

This library supports serial communication with XBee devices connected to:
 - STM32WB55 Nucleo Pack
    - Nucleo Board: STM32WB55RGV6
    - USB Dongle: STM32WB55CGU6
 - Heltec Wireless Stick v2 (ESP32)

Compile the code with PlatformIO, the platformio.ini file has definitions for both board architectures. Requires the SafeString library. Installation on Wireless Sticks supports LoRaWan (WIP) and needs additionally the ESP32_LoRaWAN libraray provided by Heltec. Heltec doesn't support their products well. In case of an error, add a line to _.pio/libdeps/heltec_wireless_stick/ESP32_LoRaWAN/src/Mcu.S_:

Add __retw.n__ under the __call8   getLicenseAddress__ (should be in line 86)
