# USB2WirelessManager

`USB2WirelessManager` opens `usb2ethernet.device` and detects the adapter
registered on the selected `UNIT`. RTL8188EU supports open, WPA-Personal
(TKIP), WPA2-Personal and WPA3-Personal networks. RT2870 supports the three
WPA modes. SiS163U supports [passive scanning](#sis163u-passive-scan).
Use the unit shown in the serial log, normally 0 for the first adapter.

## Build and install

Build from the repository root with the AmigaOS 4 cross-compiler:

```sh
make -C Libraries/WPA
make -C Libraries/WPA2
make -C Libraries/WPA3
make -C Libraries/WiFi
make -C Programs/USB2WirelessManager
```

Boot with the matching `usb2.kmod`. Follow the
[RTL8188EU guide](RTL8188.md) or [RT2870 guide](RT2870.md) to install the
class driver and firmware. Run
`Programs/USB2WirelessManager/bin/USB2WirelessManager` from a Shell. The distribution archive places the program at
`usb2/Tools/USB2WirelessManager`.

For legacy WPA-Personal, copy `usb2wpa.library` to `LIBS:`. For WPA2, copy `Libraries/WPA2/bin/usb2wpa2.library` to `LIBS:`. The archive
includes it at `usb2/Libs/usb2wpa2.library`. For WPA3, copy
`usb2wpa3.library` version 1.4 or later to `LIBS:`. Install
[AmiSSL](https://github.com/jens-maus/amissl/releases) v5 (5.16+) or v6
(6.1+) as well.

## Use

```text
USB2WirelessManager "My Network"
USB2WirelessManager "My Network" OPEN
USB2WirelessManager "My Network" WPA
USB2WirelessManager "My Network" WPA2
USB2WirelessManager "My Network" WPA3
USB2WirelessManager "My Network" WPA2 CHANNEL=5
USB2WirelessManager "My Network" WPA2 UNIT=1
USB2WirelessManager "My Network" UNIT=0 CHANNEL=5 AUTH
USB2WirelessManager "My Network" UNIT=0 CHANNEL=5 ASSOCIATE
USB2WirelessManager "My Network" UNIT=0 WPA2 PASSWORD="example passphrase"
USB2WirelessManager "My Network" UNIT=0 WPA3 PASSWORD="example passphrase"
USB2WirelessManager "My Network" UNIT=0 WPA2 WPA3
```

With only an SSID, the program scans the adapter's channels, reports a matching access
point's address, channel, and security, then exits. `OPEN` joins an open
network. `WPA` explicitly selects legacy WPA-Personal with TKIP. It requires
`usb2wpa.library`, with RTL8188EU 1.3 and manager 0.21 or later, or
RT2870 1.22 and manager 0.23 or later. Use `WPA` on its own, with an 8–63 character passphrase.
On mixed networks, select `WPA2` when the access point offers CCMP for both
pairwise and group traffic. `WPA2` joins a WPA2-Personal network using AES-CCMP and accepts a
passphrase of 8–63 printable ASCII characters. Supply `PASSWORD="..."`
with `WPA`, `WPA2` or `WPA3` to skip the prompt. The passphrase can appear in Shell history
or in the script that supplies it. When `PASSWORD` is omitted, the manager
prompts in an interactive Shell and hides the characters as you type.

`WPA3` requires an access point offering SAE group 19, CCMP-128 and BIP-CMAC-128.
Use RTL8188EU 1.2 or later with USB2WirelessManager 0.20 or later, or use RT2870
1.21 or later. The manager selects
Hash-to-Element when advertised. Older access points can use
Hunting-and-Pecking. This
version supports SAE passwords of 1–63 printable ASCII characters.
APs that require an SAE anti-clogging token or a password identifier
need additional support.

Use both `WPA2 WPA3` to accept either mode. The manager selects the first
matching access point with compatible security and prefers WPA3 if that
access point and the selected driver support it. A failed WPA3 exchange
ends the attempt. `WPA3` by itself requires WPA3.

`CHANNEL` limits the scan to one channel from 1 to 11 on RTL8188EU, or 1
to 13 on RT2870. `UNIT` selects a SANA-II unit from 0 to 31 and defaults to 0. Use the same unit number in the
TCP/IP configuration. The access point must advertise its SSID for passive
scanning.

On RT2870, `AUTH` sends an 802.11 authentication request to the matching
access point and reports its reply. `ASSOCIATE` tests WPA2-Personal
association and waits for message 1 of the WPA2 handshake. These tests
finish before password entry. `WPA2` brings the interface online after
the handshake. Use
`usb2ethernet.device` and the same unit in your TCP/IP configuration.

Wait for `WPA connected`, `WPA2 connected`, `WPA3 connected`, or `Associated` before starting the TCP/IP
interface. Leave the manager running while connected so it can handle group-key updates. Press Ctrl-C to disconnect.

If the Shell only reports `Found ... (protected)`, scanning succeeded but the
network has not been joined. Run with `WPA2` or `WPA3` to match the access point and enter the passphrase. If the
driver reports `S2_CONFIGINTERFACE error 4, wire error 15`, another client has
already configured that unit; the manager accepts this state. A driver log
with `AID 0, online 0` means association has not completed yet.

## SiS163U passive scan

SiS163U support is experimental. The passive scan found zero SSIDs on the
tested Topcom adapter.

Install `eth_SiS163U.Class` and `eth_SiS163U.Driver` in `LIBS:USB2/`.
Run the scan with USB2WirelessManager 0.36 or later and the Topcom
`0D8E:0163` attached.

```text
USB2WirelessManager SCAN UNIT=0
USB2WirelessManager SCAN UNIT=0 CHANNEL=9
```

The scan listens on channels 1 through 13 unless you select one channel.
It lists the SSIDs heard and works without a configured TCP/IP interface.
Unit 0 is the default. Association and data transfer are still in development.

## Saved profiles and startup

Install `usb2wifi.library` 1.27 or later in `LIBS:`.
Starting with RTL8188EU driver version 1.14 and RT2870 driver version 1.31, Wi-Fi
setup starts when the adapter registers with `usb2ethernet.device`.
The library connects without a window if its saved `ETH:` record has
autoconnect enabled and the login succeeds. If no profile matches, autoconnect
is disabled, the connection fails, or a required password is missing, the
picker opens. Networks appear as they are found. With the matching stack and
`usb2wifi.library` 1.30, the picker and password window show the adapter's
`.Class` title.

Run `USB2WirelessManager` with no arguments to open the profile editor.
Use the **SSIDs** and **Devices** tabs to add, remove, or edit entries.
Double-clicking an entry opens its editor. To remove an SSID referenced
by a device, change or remove that device's entry first.
**Use** writes the edits to `ENV:usb2wifi` for this boot. **Save** writes
them to both `ENV:usb2wifi` and `ENVARC:usb2wifi` so they survive a reboot.
**Cancel** and the window close button discard unsaved changes. The editor
requires manager 0.34 or later. To open the picker explicitly, end any active connection
on that adapter and use `SETUP`.

```text
USB2WirelessManager SETUP
USB2WirelessManager SETUP UNIT=0
```

Use Up and Down to select a network, fill its fields and move the cursor
to the end of **Password**. Tab and Shift+Tab move between the fields.
Return or keypad Enter moves from **SSID** to **Password** and connects
from **Password**. Selecting a network fills the **SSID** field. The **Security**
column lists the modes the adapter can use, such as `WPA2 WPA3`. The picker
chooses WPA3 when both modes are available. Selecting a protected network
fills the masked password field if the saved password for that SSID is
valid for the chosen mode. The picker opens with empty fields
and selects the last successful SSID for the current adapter and unit when
that network appears, then focuses **Password**. You can use the list or
arrow keys to choose a network while the scan runs.
For a hidden network, type its exact name in
**SSID**. The manager sends a directed probe to discover its security.
Enter the password and choose **Connect**. Leaving the password blank
reuses a saved password that fits the connection or opens a password
prompt when one is needed.

A successful connection caches the network, channel and password in
`ENV:usb2wifi` for the current boot. The file is ASCII and stores
network credentials separately from each adapter's last connection.

```text
# SSID: name, password, hidden
SSID: "My Network", "example-password", 0

# ETH: adapter name, last SSID, last security, last channel, autoconnect, factory MAC
ETH: "RTL8188EU Wi-Fi", "My Network", WPA2, 6, 0, "02:11:22:33:44:55"
```

An `ETH:` line references an `SSID:` line by name.
The saved adapter name comes from its `.Class` title. The factory MAC
preserves the selection across title or unit number changes.
The manager opens `usb2ethernet.device` using the registered unit number.
The optional `DEVICE` selector accepts the registered `.Class` title, driver
filename, or alias,
for example `USB2WirelessManager SETUP DEVICE="RTL8188EU Wi-Fi" UNIT=0`.
Use `UNIT` to select the adapter when more than one is attached.
Adapters using the same SSID share its password. The adapter entry saves
the security mode used for its last successful connection. Use `OPEN`, `WPA`,
`WPA2`, or `WPA3` for security, channel `0` when unknown, and `""` for
an empty password. Hidden and autoconnect flags are `0` or `1`.
Set autoconnect to `1` to try the saved network when the interface starts.
In quoted fields, use `\"` for a quote, `\\` for a backslash, and
`\xHH` for a byte outside printable ASCII. Blank lines and lines
starting with `#` are allowed. The password is readable in the file.

Use the same `UNIT` number as your `usb2ethernet.device` TCP/IP interface.
`AUTO` uses the same autoconnect setting as normal startup. The
following commands start Wi-Fi from a Shell task.

```text
Run <NIL: >NIL: USB2WirelessManager AUTO
Run <NIL: >NIL: USB2WirelessManager AUTO UNIT=0
```

Use the line for the adapter you want to start. Keep the manager running
while connected.
It tries the last successful channel first and scans the rest if the network
has moved. Configure TCP/IP with the same SANA-II device and unit. If your
TCP/IP setup times out while Wi-Fi is waiting for a password, start the
interface after the manager reports that it is connected.

If the key exchange fails and can be retried, the full picker reopens with
the networks it already found and focuses the blank **Password** field for
the failed SSID. Newly discovered networks appear as the scan runs. You can
enter a new password or select another network, then choose **Connect** again.
Close the picker or press **Escape** there to cancel the attempt. If a
protected network needs a password, a password prompt opens. Type the password
and press **Enter** to connect. Press **Escape**, close the window, or press
**Enter** with an empty password to return to the picker. The library caches
a password after authentication succeeds.
