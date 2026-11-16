# Kitchen LED controller

This project runs on an ESP8266EX with 4 MB flash, using ESP8266 RTOS SDK v3.4.
It controls four independent WS2812 LED lines and one active-high relay output.

## Features

- Each LED line defaults to 15 LEDs. The LED counts and output pins are configurable.
- The controller starts with all LEDs black and the relay off.
- The relay uses GPIO14, and the onboard blue LED on GPIO16 indicates when it is on.
- The web interface has a Controller tab with a relay switch and a separate OTA tab.
- Wi-Fi starts in station mode and falls back to an access point if it cannot obtain an IP address.
- mDNS provides the local address `http://kitchen-led.local/`.
- Firmware updates work through a web page or the CLI, using two OTA slots and an access token.

## Current status

Wi-Fi and OTA updates have been tested on the board.
Wi-Fi credentials are configured at build time.
The LED and relay APIs and an optional GPIO5 test effect are available in firmware.
The web interface controls the relay. LED line controls and scheduling are not implemented yet.
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
