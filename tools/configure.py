#!/usr/bin/env python3
"""Prepare local Wi-Fi credentials and the OTA access token."""

import argparse
import json
from pathlib import Path
import re
import secrets

ROOT = Path(__file__).resolve().parent.parent
LOCAL = ROOT / ".secrets"


def prepare(apply_wifi=False):
    LOCAL.mkdir(mode=0o700, exist_ok=True)
    wifi_path = LOCAL / "wifi.json"
    if not wifi_path.exists():
        wifi_path.write_text(json.dumps({"ssid": "", "password": ""}, indent=2) + "\n")
    wifi_path.chmod(0o600)

    token_path = LOCAL / "ota_token.txt"
    if not token_path.exists():
        token_path.write_text(secrets.token_hex(32) + "\n")
    token_path.chmod(0o600)
    token = token_path.read_text().strip()
    if not re.fullmatch(r"[0-9a-f]{64}", token):
        raise ValueError("The OTA token must contain 64 lowercase hexadecimal characters.")
    header = LOCAL / "ota_secret.h"
    header.write_text('#pragma once\n#define KITCHEN_OTA_TOKEN "' + token + '"\n')
    header.chmod(0o600)

    if apply_wifi:
        wifi = json.loads(wifi_path.read_text())
        ssid, password = wifi["ssid"], wifi["password"]
        if not isinstance(ssid, str) or not 1 <= len(ssid.encode()) <= 32:
            raise ValueError("The station SSID must contain 1 to 32 bytes.")
        if not isinstance(password, str):
            raise ValueError("The station password must be a string.")
        if password and not (8 <= len(password.encode()) <= 63 or
                             re.fullmatch(r"[0-9a-fA-F]{64}", password)):
            raise ValueError("The station password must contain 8 to 63 bytes, or a 64-digit hexadecimal key.")
        if any(c in ssid + password for c in "\r\n\0"):
            raise ValueError("Wi-Fi credentials cannot contain newlines or NUL characters.")

        config_path = ROOT / "sdkconfig"
        config = config_path.read_text() if config_path.exists() else ""
        values = {
            "KITCHEN_WIFI_SSID": ssid,
            "KITCHEN_WIFI_PASSWORD": password,
        }
        for key, value in values.items():
            config = re.sub(r"^CONFIG_" + key + r"=.*\n?", "", config, flags=re.MULTILINE)
            config += "\nCONFIG_" + key + "=" + json.dumps(value, ensure_ascii=False) + "\n"
        config_path.write_text(config)
        config_path.chmod(0o600)
        print("Local Wi-Fi credentials applied to sdkconfig.")
    else:
        print("Local configuration is ready in .secrets/. Credentials and tokens are not printed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply-wifi", action="store_true")
    args = parser.parse_args()
    prepare(args.apply_wifi)
