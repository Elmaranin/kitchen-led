# Wi-Fi

Set the station SSID and password under **Kitchen LED controller → Wi-Fi** in `menuconfig`.
The station uses DHCP and requires a 2.4 GHz network.
Station mode disables Wi-Fi power saving to keep update transfers responsive.
An empty password selects an open network. A configured password requires WPA2 support.
Credentials are compiled into the firmware. The generated `sdkconfig` file is ignored by Git.

After output initialization, the controller tries to obtain a station IP address.
It retries failed connection attempts at intervals of at least two seconds.
If it has no IP address after 30 seconds, it switches to AP mode.
The timeout is configurable. It also applies if an established station connection is lost.
If the SSID is empty, the controller starts the AP immediately.

The fallback AP uses these settings:

| Setting | Value |
| --- | --- |
| Name | `kitchen-led-XXXXXX`, using the last three bytes of the AP MAC address |
| Security | WPA2-PSK |
| Default password | `kitchen-led-setup` |
| Default address | `192.168.4.1` |
| Maximum clients | 4 |

Set your fallback AP password in `menuconfig`; it must contain 8–63 bytes.
The serial log shows the AP name and address, or the assigned station address.
The application does not log passwords.
Fallback AP mode remains active until reboot. Rebooting starts a new station connection attempt.
The firmware update page is available over either station Wi-Fi or the fallback AP.
A page for changing Wi-Fi credentials is not implemented yet.

## Local hostname

The controller advertises `http://kitchen-led.local/` through mDNS and publishes an `_http._tcp` service on port 80.
The responder follows station and access-point network events, including reconnects and fallback to AP mode.
Use **Kitchen LED controller → Wi-Fi → mDNS hostname** in `menuconfig` to change the name.
Enter the name without `.local`. Use letters, digits, and internal hyphens, with a maximum length of 63 characters.
Choose a different name for each controller on the network.

Your computer or phone must be on the same local network, and the network must allow multicast traffic.
If mDNS is unavailable, use the station IP from the serial log or `192.168.4.1` while connected to the fallback AP.
An mDNS setup failure is logged and does not prevent Wi-Fi or IP-based access.

[Project overview](../README.md)
