# ZigWS2812_controller

With this, you will control any kind of WS2812 Leds with zigbee protocol.
If you get a Philips Hue or ZiGate gateway, you could play with your strip or other LEDs stuff

This project use NXP JN5168 ZigBee module.

PS : this project is a proof of concept yet


## Version v0.2

* Add button compatibility for reset factory
* Fix number of LEDs --> 720 drivable LEDs
* Fix WS2812 driver (stability)
* Fix color for identify command

## Version v0.1

* Add Philips Hue differents key to be compatible
* Add WS2812 driver in Makefile
* Modify module to JN5168 in Makefile
* Link to the right SDK  JN-SW-4168

## Version initiale
This is the JN-AN-1171-ZigBee-LightLink-Demo application note from NXP website
