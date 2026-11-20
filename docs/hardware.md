# Hardware

## Outputs

| Output | ESP8266 GPIO | Default LED count | LED count parameter |
| --- | --- | --- | --- |
| LED line 1 | 4 (D2) | 36 | `CONFIG_KITCHEN_LINE_1_LEDS` |
| LED line 2 | 5 (D1) | 36 | `CONFIG_KITCHEN_LINE_2_LEDS` |
| LED line 3 | 12 (D6) | 36 | `CONFIG_KITCHEN_LINE_3_LEDS` |
| LED line 4 | 13 (D7) | 36 | `CONFIG_KITCHEN_LINE_4_LEDS` |
| Relay | 14 (D5) | N/A | N/A |
| Blue LED (`BLUE_LED`) | 16 (D0) | N/A | N/A |

Each line has a separate LED count parameter, with a supported range of 1–144 LEDs.
Set these parameters under **Kitchen LED controller** in `menuconfig`, then rebuild the firmware.
The project defaults are also recorded in `sdkconfig.defaults`.
The LED outputs form two adjacent header pairs: **D1–D2** and **D6–D7**.

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
The installed controller uses a **74HCT125D** to convert the ESP8266's 3.3 V data outputs to 5 V LED data signals.
Its four buffer channels drive the four LED lines.
Power the buffer from 5 V and connect its ground to both the controller ground and the strip power ground.
Connect each GPIO to a buffer A input and its corresponding Y output to the strip's DIN.
Tie each active-low OE input to ground to enable that channel.
The HCT inputs accept TTL levels. The D suffix identifies the SO14 package.
See the [74HCT125D datasheet](https://assets.nexperia.com/documents/data-sheet/74HC_HCT125.pdf) for pin assignments and electrical limits.
Direct 3.3 V data may work with some strip revisions, but reliable operation depends on the strip's input threshold and wiring.
Use a relay module with a compatible logic input, or a suitable transistor driver for a bare relay.
GPIO14 does not support an internal pull-down; firmware drives it low after initialization.
[GPIO16 is the only ESP8266 GPIO with an internal pull-down.](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/peripherals/gpio.html)
If the relay must stay off during reset, use an external pull-down or an input circuit that provides that state independently.

## WS2812B strip compatibility

Some WS2812B-V5 revisions require a zero-bit high pulse between 220 ns and 380 ns.
See the [Worldsemi datasheet](https://www.world-semi.co.kr/_files/ugd/89cd03_1023b0e9d135431aa1e6491bfc318112.pdf).
The transmitter now uses a shorter zero-pulse threshold and samples its timing reference before raising the outputs.
The previous timing could make zero bits appear as ones, which is a possible cause of addressed pixels showing white.
The change still needs waveform validation on the connected strip; building the firmware does not measure pulse timing.

The current test adapter is rated at 5 V and 30 W, or 6 A.
Keep the identification test dim. Size the supply for the actual strip's requirements before using continuous full-white output.

## Confirmed installation

The original 144-pixel strip was cut into four 36-pixel strips.
The installed data wires are approximately 1 m long.
Direct 3.3 V signaling produced flicker and incorrect colors on this wiring.
The user confirmed expected operation with the 74HCT125D level converter and firmware **0.3.18** on 2026-11-20.

The tested firmware uses RGB byte encoding with **Swap red and green** enabled on all four lines.
Together, these settings send GRB bytes on the wire.
The Controller tab retains the color selector and individual line identification controls.

## Relay module and load limit

The relay is an **Omron G3MB-202P** solid-state relay.
The maximum connected load for this project is **300 W**. Do not connect a load above this limit.
The controller has no power sensor and cannot measure or enforce the load limit automatically.

[Project overview](../README.md)
