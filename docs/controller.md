# Controller interface

Open `http://kitchen-led.local/` on the same network as the controller.
You can also use `http://CONTROLLER_IP/` if mDNS is unavailable.
The page opens the **Controller** tab. The **OTA** tab contains firmware update controls.

## Relay

The relay switch controls the active-high output on GPIO14 (D5).
The relay is an Omron G3MB-202P. Its connected load must not exceed the project's **300 W** limit.
The interface displays this limit; the controller cannot measure load power or shut down based on wattage.
The onboard blue LED on GPIO16 (D0) lights when the relay is on and goes dark when it is off.
The relay starts off after every boot. Its state is not saved to flash.
The page reads the current state before enabling the switch and refreshes it every five seconds.
Reloading the page or switching tabs does not change the output.

The switch shows the last state confirmed by the controller.
The controller reports the commanded GPIO state; it does not measure the relay contacts.
If a command fails, the page reads the state again and shows an error.
Controls are disabled while the device is unavailable or an OTA update is in progress.

Relay controls are available to devices on the local network and do not require the OTA token.
OTA uploads remain protected by their access token.
LED line controls and scheduling are not implemented yet.

## HTTP API

`GET /status` includes a `relay_on` boolean alongside the existing firmware information.

`POST /relay` accepts a JSON boolean and requires `Content-Type: application/json`.
It sets the requested state explicitly, so repeating a command does not toggle the relay.
The server does not allow cross-origin browser requests.

```sh
curl -H 'Content-Type: application/json' --data 'true' http://CONTROLLER_IP/relay
curl -H 'Content-Type: application/json' --data 'false' http://CONTROLLER_IP/relay
```

The response is `{"on":true}` or `{"on":false}` after a successful GPIO write.
Invalid or incomplete commands leave the relay unchanged.

[Project overview](../README.md)
