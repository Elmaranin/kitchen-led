# Firmware API

Call the API after `kitchen_led_init()` succeeds. Keep LED buffer updates and frame transmission in one task.
Line and pixel indices start at zero. Colors use RGB values from 0 to 255.
Pixel changes are buffered until `kitchen_led_show()` sends them to the LEDs.

```c
ESP_ERROR_CHECK(kitchen_led_fill(0, 16, 16, 16));
ESP_ERROR_CHECK(kitchen_led_set_pixel(1, 0, 16, 0, 0));
ESP_ERROR_CHECK(kitchen_led_show());
ESP_ERROR_CHECK(kitchen_relay_set(true));
```

The default application initializes the outputs, starts Wi-Fi, and serves the firmware update page.
The Controller tab accepts relay commands through the [HTTP API](controller.md#http-api).
`kitchen_relay_is_on()` returns the last successfully commanded relay state.
`kitchen_relay_set()` drives the relay on GPIO14 and the active-low blue LED on GPIO16 together.
The HTTP server handles relay writes in one task. Keep relay writes serialized when adding other control sources.

## LED strip test

Enable **Run the four-line LED identification test** under **Kitchen LED controller** in `menuconfig`.
Set each line's LED count to match the connected strip, such as **50** or **144** pixels.
Each line defaults to 36 LEDs. The test uses each line's configured count.

| Line | GPIO / board pin | Identification color |
| --- | --- | --- |
| 1 | GPIO4 / D2 | Green |
| 2 | GPIO5 / D1 | Blue |
| 3 | GPIO12 / D6 | Red |
| 4 | GPIO13 / D7 | White |

The test runs automatically after initialization. Each line keeps its assigned color throughout the chase.
At startup, each chase moves one pixel every 100 ms with a short fading tail and 6% brightness (15 out of 255).
The Controller tab can change the speed and brightness of all four chases through the [HTTP API](controller.md#led-effect).
The effect reads one consistent settings snapshot per frame. Only the effect task changes the LED buffers.
The four lines share one frame transmission. The effect does not change the relay.
Wi-Fi and OTA remain available during the effect.
Disable the test option and rebuild to return to normal startup behavior.

## Transmitter timing

The transmitter sends data on four GPIOs in parallel from instruction RAM.
**LED color byte order** in menuconfig selects RGB or GRB for all four lines.
The project defaults to RGB encoding with red and green swapped on all four lines, which sends GRB bytes.
The C API always accepts red, green, and blue in that order.
Each line has a **Swap red and green** setting that reverses those two channels before byte encoding.
These defaults match the installation verified with firmware 0.3.18 and the 74HCT125D level converter.
It uses the CPU cycle counter and masks interrupts during each frame.
At the default length of 36 LEDs, a frame takes approximately 1.2 ms, plus two 300 µs reset intervals.
With 50 LEDs per line, the parallel frame takes approximately 1.7 ms, plus those reset intervals.
With 144 LEDs per line, the frame takes approximately 5 ms, plus those reset intervals.
During the pulse train, the SDK defers Wi-Fi MAC interrupts and keeps the watchdog timer enabled.
The reset intervals allow interrupts. Shorter lines receive black padding up to the longest line.
The chase requests at most 50 frames per second to give Wi-Fi time between frames.
At the maximum speed of 20×, the head moves four pixels per frame when the chase runs at 50 frames per second.
The timing routine requires a fixed 80 MHz CPU clock.
Its zero-pulse comparison threshold is 20 CPU cycles, or 250 ns, before branch and GPIO-write overhead.
The timing reference is sampled before the rising edge. Actual pulse widths require measurement on the hardware.
The transmitter samples the CPU clock again just after the GPIO write that clears zero bits.
If any sample takes at least 600 ns, it finishes the frame, waits for the reset interval, and sends it once more.
The Controller tab and the `led_late_frames` field in `/status` count frames retried since boot.
This diagnostic can miss smaller delays and electrical errors at the strip input.
Check the waveforms and Wi-Fi stability with a logic analyzer and the actual LED lines before installation.

[Project overview](../README.md)
