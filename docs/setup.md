# Setup, build, and USB flashing

## SDK

This project uses Espressif's separate
[ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK).
Use release **v3.4**, with its matching Xtensa LX106 compiler.
Its build system follows the ESP-IDF style, but ESP32 ESP-IDF does not target ESP8266.

ESP-IDF v6.1 is installed locally at `~/.espressif/v6.1/esp-idf`.
That installation cannot build this firmware.
The connected ESP8266EX has 4 MB flash. The build reserves two 960 KiB firmware slots using the SDK's OTA partition table.

## Build and flash

Follow the SDK's [installation guide](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/v3.4/get-started/index.html)
to install v3.4 and its compiler in a separate SDK directory.
For example, clone the SDK and initialize its direct submodules:

```sh
git clone --depth 1 --branch v3.4 https://github.com/espressif/ESP8266_RTOS_SDK.git "$HOME/esp/ESP8266_RTOS_SDK"
git -C "$HOME/esp/ESP8266_RTOS_SDK" submodule update --init
```

The application does not use CoAP or its nested TinyDTLS submodule.
The nested TinyDTLS URL in this old release redirects to a different host, so recursive checkout can fail.
Use a separate Python environment for this SDK's older dependencies; do not reuse the ESP-IDF v6.1 environment.
Python 3.10 and CMake 3.x are suitable for the build tools.

Before the first build, run `python3 tools/configure.py` from the project root.
This creates an ignored `.secrets` directory with a Wi-Fi configuration file and a random OTA token.
To use station mode, fill in `.secrets/wifi.json`, then run `python3 tools/configure.py --apply-wifi`.
The script applies the credentials to `sdkconfig` without printing them.
Keep `.secrets/ota_token.txt` for future updates. The generated `ota_secret.h` embeds the same token in each firmware build.

In a fresh shell, activate that SDK's environment, then run from this project:

```sh
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
. "$IDF_PATH/export.sh"
idf.py -B build/verified menuconfig
idf.py -B build/verified build
idf.py -B build/verified -p /dev/cu.YOUR_SERIAL_PORT -b 57600 flash monitor
```

Set the pins and LED counts under **Kitchen LED controller** in `menuconfig`.
Each line supports 1–144 LEDs. Use four distinct pins from GPIO4, GPIO5, GPIO12, and GPIO13.
GPIO14 is reserved for the relay, and GPIO16 is reserved for the blue LED indicator.
The application rejects flash pins and duplicate pins during initialization.
Keep the CPU at 80 MHz and power management disabled, as set in `sdkconfig.defaults`.
The connected USB serial adapter passed write verification at 57600 baud; faster transfers produced corruption during testing.
The SDK console runs at 74880 baud, independently of the flashing speed.

## VS Code tasks

The workspace includes three tasks in [`.vscode/tasks.json`](../.vscode/tasks.json):

- **Build firmware** builds `build/verified/kitchen_led.bin`. Press **Cmd+Shift+B** on macOS to run it.
- **Flash firmware (USB)** builds first, then prompts for the serial port and flashes at 57600 baud.
- **Flash firmware (OTA)** prompts for the controller address, then builds and uploads in one terminal using the saved OTA token.

The OTA address defaults to `kitchen-led.local`.
For the first update that enables mDNS, replace it with the device's current IP address.
If you configure another hostname, enter that name with the `.local` suffix instead.

Run either flash task through **Terminal → Run Task**.
The OTA task prints **Step 1/2: Building firmware** and **Step 2/2: Uploading firmware over Wi-Fi** in the same terminal.
Wait for **OTA verified** to confirm that the controller restarted in the other firmware slot.
These tasks select the ESP8266 SDK independently of the ESP-IDF extension's selected SDK.
They use the verified SDK, Python environment, and compiler under `/tmp/kitchen-led-*`.
If those directories move or are removed, update their paths in `tasks.json`.

[Project overview](../README.md)
