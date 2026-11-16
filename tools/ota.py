#!/usr/bin/env python3
"""Upload a firmware image to a controller on the local network."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parent.parent


def build_and_update(base, image, token_path):
    sdk = os.environ.get("IDF_PATH")
    if not sdk or not (Path(sdk) / "tools/idf.py").is_file():
        raise ValueError("Set IDF_PATH to the ESP8266 RTOS SDK before building.")
    print("Step 1/2: Building firmware...", flush=True)
    subprocess.run([sys.executable, str(Path(sdk) / "tools/idf.py"),
                    "-B", str(image.resolve().parent), "build"], cwd=ROOT, check=True)
    print("Step 2/2: Uploading firmware over Wi-Fi...", flush=True)
    update(base, image, token_path)


def status(base):
    with urllib.request.urlopen(base + "/status", timeout=5) as response:
        return json.load(response)


def update(base, image, token_path):
    before = status(base)
    payload = image.read_bytes()
    if not payload or payload[0] != 0xE9:
        raise ValueError("The file does not have an ESP firmware image header.")
    token = token_path.read_text().strip()
    request = urllib.request.Request(
        base + "/update", data=payload, method="POST",
        headers={"Authorization": "Bearer " + token,
                 "Content-Type": "application/octet-stream"})
    print(f"Uploading {len(payload)} bytes. Current firmware: {before['version']} in {before['slot']}.")
    try:
        with urllib.request.urlopen(request, timeout=150) as response:
            print(response.read().decode().strip())
    except urllib.error.HTTPError as error:
        raise RuntimeError(f"Upload rejected ({error.code}): {error.read().decode()}") from None

    deadline = time.monotonic() + 90
    while time.monotonic() < deadline:
        time.sleep(2)
        try:
            after = status(base)
            if not after["reboot_pending"] and after["slot"] != before["slot"]:
                print(f"OTA verified: {after['version']} is running in {after['slot']}.")
                return
        except (OSError, ValueError):
            pass
    raise RuntimeError("The upload was accepted, but the new firmware could not be confirmed after reboot.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", help="Controller IP address or hostname")
    parser.add_argument("image", type=Path)
    parser.add_argument("--token-file", type=Path, default=ROOT / ".secrets/ota_token.txt")
    parser.add_argument("--build", action="store_true", help="Build before uploading firmware")
    args = parser.parse_args()
    base = "http://" + args.host
    if args.build:
        build_and_update(base, args.image, args.token_file)
    else:
        update(base, args.image, args.token_file)
