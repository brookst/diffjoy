=======
DiffJoy
=======

Description
-----------

The DiffJoy is a one-axis, USB HID joystick based around an Atmel AVR ATTiny45 microcontroller and the Object Development AVR-USB firmware driver.
The code for DiffJoy is licensed under the GNU GPLv2.  You can get more info in LICENSE.txt

A Python systemd service is included that translates positions into key presses.
Data is read from `/dev/hidraw*` and key presses generated with the Python `keyboard package <https://pypi.org/project/keyboard>`_
.
This currently sends angle brackets to speed up and slow down web video playback speed.
The flake.nix file provides a NixOS module to add a systemd service.

Installation Instructions
-------------------------

Firmware
========
Ensure gcc-avr is available on your system.

Run make in the ``src`` directory::

$ make

To flash a chip with AVRDUDE, run::

$ make fuse && make flash

Software Service
================
For non-flake system configurations, add the default module to your imports and enable the service::

  {
    imports =
      [
        (builtins.getFlake
          "github:brookst/diffjoy"
        ).nixosModules.default
      ];
    services.pedal_controller.enable = true;
  }

Files
-----

* LICENSE.txt - GNU GPLv2
* README.rst - this file
* src - project source
* src/usbdrv - the V-USB driver: http://www.obdev.at/products/vusb
* flake.nix - Nix flake for Python service, devShell and NixOS module
* pyproject.toml - Python module packaging data
* pedal_controller/ - Python pedal_controller script
