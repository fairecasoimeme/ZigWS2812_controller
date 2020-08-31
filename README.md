# ZigWS2812_controller

With this, you will control any kind of WS2812 Leds with zigbee protocol.
If you get a Philips Hue or ZiGate gateway, you could play with your strip or other LEDs stuff

This project use NXP JN5168 ZigBee module.

Vidéo : 
[![Watch the video](https://img.youtube.com/vi/ZlssjOw1DXE/maxresdefault.jpg)](https://youtu.be/ZlssjOw1DXE)

<div ><img width="300px" src="https://github.com/fairecasoimeme/ZigWS2812_controller/blob/master/Lifesmart%20cololight%20controled.jpg" /><br><img width="300px" src="https://github.com/fairecasoimeme/ZigWS2812_controller/blob/master/Screenshot%20philips%20hue%20appairage.jpg" /></div>

## Install
In progress

## Hardware
In progress

## Version v0.4

* Fix twinkle leds
* Fix radio sensivity and txPower

## Version v0.3

* Add blue led status. Specially blink led when commissioning mode is actived or when device has joined network
* Add button function. Simple clic toggle the WS2812 LEDs
* Fix channels to 11,15,20,25,26

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
