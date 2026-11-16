# Firmware API

Call the API from one task after `kitchen_led_init()` succeeds.
Line and pixel indices start at zero. Colors use RGB values from 0 to 255.
Pixel changes are buffered until `kitchen_led_show()` sends them to the LEDs.

```c
ESP_ERROR_CHECK(kitchen_led_fill(0, 16, 16, 16));
ESP_ERROR_CHECK(kitchen_led_set_pixel(1, 0, 16, 0, 0));
ESP_ERROR_CHECK(kitchen_led_show());
ESP_ERROR_CHECK(kitchen_relay_set(true));
```

The default application initializes the outputs, starts Wi-Fi, and serves the firmware update page.
It does not accept external LED and relay control commands yet.

## LED strip test

Enable **Run the GPIO5 LED test effect** under **Kitchen LED controller** in `menuconfig`.
Set **Line 2 GPIO** to **5** and **Line 2 LED count** to **50** for the connected test strip.
The normal project defaults remain 15 LEDs per line; the test uses the configured line 2 count.

The test runs automatically after initialization and repeats a red, green, and blue chase.
It moves one pixel every 100 ms with a short fading tail and a peak channel value of 16 out of 255.
The other lines stay black and the relay stays off.
Wi-Fi and OTA remain available during the effect.
Disable the test option and rebuild to return to normal startup behavior.

## Transmitter timing

The transmitter sends GRB data on four GPIOs in parallel from instruction RAM.
It uses the CPU cycle counter and masks interrupts during each frame.
At the default length of 15 LEDs, a frame takes approximately 0.5 ms, plus two 300 µs reset intervals.
The reset intervals allow interrupts. Shorter lines receive black padding up to the longest line.
The timing routine requires a fixed 80 MHz CPU clock.
Check the waveforms and Wi-Fi stability with a logic analyzer and the actual LED lines before installation.

[Project overview](../README.md)
