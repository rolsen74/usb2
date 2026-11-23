# USB2 AHI Playback

`usb2.audio` plays audio through the first available playback unit of
[`usb2audio.device`](Audio.md). It sets the AHI mixer to the USB playback
unit's sample rate and uses the **USB2: 16-bit stereo playback** mode.

## Build and install

```sh
make -C Drivers/Audio/AHI
```

1. Install the current USB2 stack and the [USB Audio driver](Audio.md).
2. Copy `Drivers/Audio/AHI/bin/usb2.audio` to `DEVS:AHI/` and
   `Drivers/Audio/AHI/bin/USB2` to `DEVS:AudioModes/`. The distribution archive
   includes both files in their respective directories under `usb2/Devs/`.
3. Restart and select **USB2: 16-bit stereo playback** in AHI preferences.

The mode file uses local ID `0x7f000001`. If that ID is already in use, rebuild
with `make -C Drivers/Audio/AHI MODE_ID=...` using a free mode ID.

AHI recording is unsupported. Mode loading and playback on physical AmigaOS 4
hardware remain unverified.

## Bluetooth speaker playback

`bluetooth2.audio` exposes a Classic Bluetooth A2DP speaker as a 48 kHz,
16-bit stereo AHI output. Bluetooth2 encodes the AHI mixer output as SBC
while the application is playing.

1. Install Bluetooth2 1.23 and cls_Bluetooth.Driver 1.20. Copy
   `Devs/AHI/bluetooth2.audio` and `Devs/AudioModes/Bluetooth2` from the
   USB2 archive to the corresponding directories under `DEVS:`, then reboot.
2. Run `USB2BluetoothTest GUI UNIT=0` with version 0.20 or later,
   select the speaker, and click
   **Use for AHI**. You can also set the controller unit and speaker address
   from the Shell.

   ```text
   SetEnv Bluetooth2-AHI 0 AA:BB:CC:DD:EE:FF
   ```

3. In AHI preferences, select **Bluetooth2: A2DP stereo playback** for the
   unit your application uses. Play a short sample. If the speaker requires
   authentication, Bluetooth2 can reuse a bond saved by
   [USB2BluetoothTest](USB2BluetoothTest.md#save-and-reuse-a-bond).

The speaker setting applies the next time the AHI mode opens. To keep it
across reboots, copy `ENV:Bluetooth2-AHI` to `ENVARC:Bluetooth2-AHI`.
The mode uses local ID `0x7f000002`. Use `make -C Drivers/Audio/AHI
BT_MODE_ID=...` to choose a different ID. Bluetooth AHI playback awaits
testing with a physical speaker.
