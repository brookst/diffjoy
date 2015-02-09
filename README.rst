DiffJoy
=======

Description
-----------

The DiffJoy is a one-axis, USB HID joystick based around an Atmel AVR ATTiny45 microcontroller and the Object Development AVR-USB firmware driver.  The code for the DiffJoy is licensed under the GNU GPLv2.  You can get more info in LICENSE.txt

Installation Instructions
-------------------------

Ensure gcc-avr is available on your system.

Run make in the ``src`` directory::

$ make

To flash a chip with AVRDUDE, run::

$ make fuse && make flash

Files
-----

* LICENSE.txt - GNU GPLv2
* README.rst - this file
* src - project source
* src/usbdrv - the V-USB driver: http://www.obdev.at/products/vusb
