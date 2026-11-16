# OTA updates

The first installation uses USB to write the bootloader, OTA partition table, and application.
Later updates upload only `kitchen_led.bin` over Wi-Fi.
The updater writes the inactive slot and validates the image before selecting it for the next boot.
It rejects invalid access tokens, empty or oversized uploads, invalid images, and firmware from a different project.
An interrupted upload leaves the current boot selection unchanged.

Open `http://CONTROLLER_IP/` for the upload page.
The station address is printed in the serial log. The fallback AP address is `192.168.4.1`.
Paste the token from `.secrets/ota_token.txt`, choose `kitchen_led.bin`, and select **Upload and restart**.
The page shows the current version and firmware slot, then checks them again after reboot.
The token stays in the page only until the upload succeeds; the page does not save it in browser storage.

You can also upload from the command line:

```sh
python3 tools/ota.py CONTROLLER_IP build/verified/kitchen_led.bin
```

The uploader reads the local token file and confirms that the controller restarts in the other slot.
To build a version that is easy to distinguish during a demonstration:

```sh
idf.py -B build/verified -D PROJECT_VER=0.2.1 build
python3 tools/ota.py CONTROLLER_IP build/verified/kitchen_led.bin
```

Use this HTTP updater on a trusted local network. HTTP does not encrypt the token or firmware.
Image validation checks the firmware format and checksum; it does not verify a publisher signature.
Automatic rollback after a new application crashes is not implemented.
Recovery from faulty firmware still needs work before permanent installation.

[Project overview](../README.md)
