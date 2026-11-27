# USB2Prefs

Copy `Prefs/USB2` from the USB2 archive to `SYS:Prefs/USB2` and run it. The tabs are Audio, Bluetooth,
Ethernet, Parallel, Printer, Serial, and Video. Open the **Unit** chooser
in the page's default group, such as **Default Audio**, to select a setting.

- **Automatic** keeps the service's normal selection.
- **First Device** uses the lowest registered compatible unit. If it is busy, the
  request can fail instead of trying a different unit.
- **Custom Device** uses the item selected with the **Custom** button. Audio
  selects an output by endpoint ID. Ethernet selects a driver by name and
  uses its lowest registered unit. The other tabs select a unit number.

**Use** applies the choices for this session, **Save** also keeps them
across reboots, and **Cancel** discards edits. These buttons do not change
open sessions.

The **Ethernet** page has **Default**, **SSIDs**, and **Devices** subtabs.
The last two manage Wi-Fi profiles. Add, edit, or remove entries there. A device's profile can select
its saved SSID and enable autoconnect. The main **Save**, **Use**, and
**Cancel** buttons also apply to the Wi-Fi edits. The integrated editor
uses `usb2wifi.library` 1.49 or later for the **Default** subtab.

## Using the defaults

- AHI uses the Audio preference for each new allocation.
- `USB2VideoTest` and `USB2BluetoothTest` use the preference when you omit
  `UNIT`. An explicit unit overrides the preference. Bluetooth First
  selection requires Bluetooth2 1.26 or later.
- Applications can open `usb2ethernet.device`, `usb2serial.device`,
  `usb2parallel.device`, `usb2printer.device`, or `usb2video.device` on
  unit **65536** (`USB2_DEFAULT_UNIT`) from a DOS process to follow the
  preference. Use that number in application or printer preferences that
  accept it. Explicit unit numbers keep their existing meaning.
- For these device opens and the Bluetooth tool, Auto uses unit 0.
  In Auto, the video tool keeps its existing camera selection.

## Reconnecting USB units

The stack matches returning devices by USB vendor ID, product ID, interface,
and serial number. A device without a usable serial must return to the same USB
port path. Mass storage also matches the logical unit number on the USB device.

- Audio, serial, and printer keep an open unit reserved while the device is
  disconnected. Submit new I/O after it returns. Interrupted transfers report
  an error to the application.
- Disk and Ethernet reuse the unit number. Existing handles remain disconnected.
  Reopen the Ethernet interface or let the disk mounter reopen the returned
  volume. Interrupted disk writes and network requests are never replayed.
- A parallel backend can use `RegisterDevice` on `usb2parallel.device` 1.7
  to reserve its open unit and route new requests after reconnect. The archive
  includes the parallel frontend.

Unit numbers can change when adapters are reconnected in a different
order. An audio endpoint without a unique serial is identified by its USB
port path. The Custom list shows the devices available when it opens.
