# Kitchen LED controller

This project runs on an ESP8266EX with 4 MB flash, using ESP8266 RTOS SDK v3.4.
It controls four independent WS2812 LED lines and one active-high relay output.
The strips form a two-by-two grid named Left up, Left down, Right down, and Right up.

## Features

- Each LED line defaults to 36 LEDs. The LED counts and output pins are configurable.
- The controller starts with all LEDs black and the relay off.
- The relay uses GPIO14, and the onboard blue LED on GPIO16 indicates when it is on.
- The Controller tab provides a relay switch, LED speed and brightness, line identification, and test colors.
- The OTA tab provides firmware uploads and a reboot button.
- Wi-Fi starts in station mode and falls back to an access point if it cannot obtain an IP address.
- mDNS provides the local address `http://kitchen-led.local/`.
- Firmware updates work through a web page or the CLI, using two OTA slots and an access token.

## Current status

Wi-Fi, OTA updates, and four 36-pixel LED lines have been tested on the board.
The installed 74HCT125D buffer converts the controller's 3.3 V data signals to 5 V for the LED strips.
Wi-Fi credentials are configured at build time.
Eight LED effects are available, including rainbow, aurora, comets, embers, stars, breathing, solid color, and identification.
Effects start at 6% brightness after boot. Scheduling is not implemented yet.
Automatic rollback after faulty firmware is not implemented yet.

## Documentation

Run the commands in these guides from the project root.

| Guide | Contents |
| --- | --- |
| [Setup and build](docs/setup.md) | SDK installation, local credentials, build configuration, and initial USB flashing |
| [Hardware](docs/hardware.md) | GPIO assignments, LED counts, relay behavior, and wiring |
| [Controller interface](docs/controller.md) | Relay switch, default state, connection behavior, and HTTP commands |
| [Firmware API](docs/firmware-api.md) | LED and relay API usage, buffering, and transmitter timing |
| [Wi-Fi](docs/wifi.md) | Station mode, connection retries, and fallback AP settings |
| [OTA updates](docs/ota.md) | Browser and CLI updates, access tokens, and update limitations |
| [Testing](docs/testing.md) | Local tests and recorded hardware verification |
