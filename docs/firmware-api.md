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
It does not run effects or accept LED and relay control commands yet.

The transmitter sends GRB data on four GPIOs in parallel from instruction RAM.
It uses the CPU cycle counter and masks interrupts during each frame.
At the default length of 15 LEDs, a frame takes approximately 0.5 ms, plus two 300 µs reset intervals.
The reset intervals allow interrupts. Shorter lines receive black padding up to the longest line.
The timing routine requires a fixed 80 MHz CPU clock.
Check the waveforms and Wi-Fi stability with a logic analyzer and the actual LED lines before installation.

[Project overview](../README.md)
