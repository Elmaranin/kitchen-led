# Testing

## Local tests

Run the connection policy tests with a host C compiler:

```sh
cc -std=c11 -Wall -Wextra -Werror -I main main/wifi_policy.c tests/wifi_policy_test.c -o /tmp/kitchen-led-wifi-policy-test
/tmp/kitchen-led-wifi-policy-test
```

These tests cover retry spacing, connection timeouts, reconnects, and timer rollover.
Run the local configuration tests with `python3 -m unittest discover -s tests -p 'test_*.py'`.
These tests check secret preservation, credential validation, and configuration escaping.
The firmware build checks the SDK integration. Wi-Fi and OTA also need testing on the board.

## Hardware verification

The connected ESP8266EX passed these checks on 2026-11-15:

- The initial USB flash passed hash verification at 57600 baud.
- Station mode joined the configured Wi-Fi network and obtained a DHCP address.
- Missing-token, oversized, invalid-image, and interrupted uploads were rejected.
- Those rejected uploads left version 0.2.0 running in `ota_0`.
- A complete Wi-Fi upload installed version 0.2.1, rebooted into `ota_1`, and reconnected to Wi-Fi.
- The command-line updater confirmed the new version and slot through `/status` after reboot.

The live test exposed two compatibility issues that the application now handles.
The SDK's HTTP receiver does not reliably classify socket timeouts, so the uploader uses a receive override based on `errno`.
The ESP8266 image stores its application descriptor at the start of the read-only data segment.
The updater checks validated segment starts instead of assuming the descriptor is in the first segment.

[Project overview](../README.md)
