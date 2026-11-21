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
Scheduling and individual line controls are not implemented yet.

## LED effect

Select an effect on the Controller tab. Choosing an effect resumes all four strips.
The effects follow the physical grid and account for the reversed left strips.
The renderer uses normalized positions to coordinate movement across the rows and central gap.

| Effect | Appearance | Uses the color selector |
| --- | --- | --- |
| Rainbow wave | A rainbow flows across the grid | No |
| Aurora | Slow green, blue, and violet curtains | No |
| Twin comets | Two colorful comets cross the rows | No |
| Ember glow | A soft orange and red glow changes over time | No |
| Twinkling stars | Warm stars fade in and out over a dark background | No |
| Breathing color | The selected color brightens and fades | Yes |
| Solid color | All four strips show the selected color | Yes |
| Identification chase | Each position runs its original four-pixel chase | Yes |

The speed and brightness sliders control all four strips together.
Changes apply when you release a slider or change its value with the keyboard.
The controller confirms each change. A page reload reads the current settings without resetting them.

Speed ranges from **0.25× to 20×** and scales animation time.
The identification chase advances one pixel every 100 ms at 1×.
For that chase, a 144-pixel pass takes about 14.4 seconds at 1× or 0.72 seconds at 20×.
Animation follows elapsed time. A late frame skips forward to the current position.
Brightness ranges from **0% to 100%**. At 0%, the controller sends black and then holds all four data pins low.
If both transmission attempts show a timing delay, the controller tries again on the next frame.
Raising the brightness resumes the animation at its current position.
Brightness scales the RGB values; it does not limit current based on a power measurement.
Each reboot restores **Rainbow wave, 1× speed, and 6% brightness**. Settings are not saved to flash.
The relay operates independently.

These controls require **Enable LED effects and controller controls**, which is enabled by default in menuconfig.
The page disables the controls if effects are disabled in the firmware.

## Define lines

Use **Define lines** on the Controller tab to locate each physical strip.
Select a line to light all 36 pixels in its identification color while the other three strips turn off.
The buttons show each strip's position and GPIO in the same two-by-two grid as the installation.
Left up is line 4, left down is line 3, right down is line 2, and right up is line 1.
The left strips run from pixel 1 at the right end to pixel 36 at the left end.
The right strips run from pixel 1 at the left end to pixel 36 at the right end.
Brightness still applies, so use a brightness above zero to see the selected line.
The selected strip shows a steady color. **Show all** resumes the selected effect on all four strips.

Use **Color** to choose red, green, blue, white, yellow, cyan, or magenta.
The chosen color applies to breathing, solid color, identification, and the steady line test.
Effects with their own palettes disable the color selector until you select a line to identify.
Choose **Line colors** to restore the original colors. Every reboot starts with **Line colors**.

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

`GET /status` also includes `effect_enabled`, `speed_percent`, and `brightness`.
It includes `define_line`, where zero shows all lines and 1 through 4 selects one line.
IDs 1, 2, 3, and 4 select right up, right down, left down, and left up respectively.
`POST /effect` accepts the optional integer `define_line` with the same range.
Commands that omit this field preserve the current line selection.
`test_color` is optional and defaults to 0 for the original line colors.
Values 1 through 7 select red, green, blue, white, yellow, cyan, and magenta.
Commands that omit this field preserve the current color selection.
`effect` is optional. IDs 0 through 7 select identification, rainbow, aurora, comets, embers,
twinkle, breathing, and solid color respectively.
`GET /status` reports the selected effect ID.
Commands that omit `effect` preserve the current effect selection.
`POST /effect` requires `Content-Type: application/json` and both integer fields:

```sh
curl -H 'Content-Type: application/json' \
  --data '{"speed_percent":200,"brightness":10}' http://CONTROLLER_IP/effect
```

`speed_percent` ranges from 25 to 2000. A value of 100 means 1× speed. `brightness` ranges from 0 to 100.
The response confirms both values. Invalid commands leave both settings unchanged.
Effect commands use the same local network access as the relay and do not require the OTA token.

[Project overview](../README.md)
