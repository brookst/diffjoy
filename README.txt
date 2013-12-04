AetherSense README
==================

Description
-----------

The AetherSense is a one-axis, no button, USB HID ultrasonic distance sensing joystick based around an Atmel AVR ATTiny45 microcontroller, a MaxBotix LV-MaxSonar-EZ4 and the Object Development AVR-USB firmware driver.  The code for the AetherSense is licensed under the GNU GPLv2.  You can get more info in LICENSE.txt

Installation Instructions
-------------------------

With avr-gcc installed in the src directory, type

$ make

To flash a chip with AVRDUDE, type 

$ make fuse && make flash

Files
-----

LICENSE.txt - GNU G
README.txt - This file
schematic.png - Build your own!
src - our source
src/usbdrv - the AVR-USB driver.  More info here: http://www.obdev.at/avrusb/
