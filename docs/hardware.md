# Hardware

## Outputs

| Output | ESP8266 GPIO | Default LED count | LED count parameter |
| --- | --- | --- | --- |
| LED line 1 | 4 | 15 | `CONFIG_KITCHEN_LINE_1_LEDS` |
| LED line 2 | 5 | 15 | `CONFIG_KITCHEN_LINE_2_LEDS` |
| LED line 3 | 12 | 15 | `CONFIG_KITCHEN_LINE_3_LEDS` |
| LED line 4 | 13 | 15 | `CONFIG_KITCHEN_LINE_4_LEDS` |
| Relay | 16 | N/A | N/A |

Each line has a separate LED count parameter, with a supported range of 1–64 LEDs.
Set these parameters under **Kitchen LED controller** in `menuconfig`, then rebuild the firmware.
The project defaults are also recorded in `sdkconfig.defaults`.

These are GPIO numbers, not the board's printed D-pin labels.
Use a board that exposes all five GPIOs, such as an ESP-12-based development board.

The firmware initializes the LEDs to black and the relay output to low.
GPIO16 uses its internal pull-down. High means on; low means off.
[GPIO16 is the only ESP8266 GPIO with an internal pull-down.](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/peripherals/gpio.html)
Do not connect GPIO16 to the reset pin in this project.

## Wiring

Power the LED lines from a supply sized for their combined load.
Connect the controller and LED power supply grounds together.
Use a suitable 3.3 V to 5 V logic buffer for the WS2812 data inputs.
Use a relay module with a compatible logic input, or a suitable transistor driver for a bare relay.
The internal pull-down is enabled by firmware; it does not guarantee an off state throughout power-up or reset.
If the relay must stay off during reset, its input circuit must provide that state independently.

[Project overview](../README.md)
