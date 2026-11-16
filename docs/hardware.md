# Hardware

## Outputs

| Output | ESP8266 GPIO | Default LED count | LED count parameter |
| --- | --- | --- | --- |
| LED line 1 | 4 | 15 | `CONFIG_KITCHEN_LINE_1_LEDS` |
| LED line 2 | 5 | 15 | `CONFIG_KITCHEN_LINE_2_LEDS` |
| LED line 3 | 12 | 15 | `CONFIG_KITCHEN_LINE_3_LEDS` |
| LED line 4 | 13 | 15 | `CONFIG_KITCHEN_LINE_4_LEDS` |
| Relay | 14 (D5) | N/A | N/A |
| Blue LED (`BLUE_LED`) | 16 (D0) | N/A | N/A |

Each line has a separate LED count parameter, with a supported range of 1–64 LEDs.
Set these parameters under **Kitchen LED controller** in `menuconfig`, then rebuild the firmware.
The project defaults are also recorded in `sdkconfig.defaults`.

These are GPIO numbers, not the board's printed D-pin labels.
Use a board that exposes all five GPIOs, such as an ESP-12-based development board.

The firmware initializes the WS2812 lines to black and the relay output on GPIO14 to low.
The onboard blue LED uses GPIO16 and is active-low. It starts off with GPIO16 driven high.
Turning the relay on drives GPIO14 high and GPIO16 low, so the blue LED lights with the relay.
Turning the relay off drives GPIO14 low and GPIO16 high.
GPIO14 and GPIO16 are reserved for these outputs. LED lines can use GPIO4, GPIO5, GPIO12, and GPIO13.
Do not connect GPIO16 to the reset pin in this project.

Move the relay control wire from GPIO16/D0 to GPIO14/D5 before installing firmware with this pin mapping.

## Wiring

Power the LED lines from a supply sized for their combined load.
Connect the controller and LED power supply grounds together.
Use a suitable 3.3 V to 5 V logic buffer for the WS2812 data inputs.
Use a relay module with a compatible logic input, or a suitable transistor driver for a bare relay.
GPIO14 does not support an internal pull-down; firmware drives it low after initialization.
[GPIO16 is the only ESP8266 GPIO with an internal pull-down.](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/peripherals/gpio.html)
If the relay must stay off during reset, use an external pull-down or an input circuit that provides that state independently.

## Relay module and load limit

The relay is an **Omron G3MB-202P** solid-state relay.
The maximum connected load for this project is **300 W**. Do not connect a load above this limit.
The controller has no power sensor and cannot measure or enforce the load limit automatically.

[Project overview](../README.md)
