# USB2 AHI Playback

`usb2.audio` plays audio through the selected USB output of
[`usb2audio.device`](Audio.md). It sets the AHI mixer to the output's sample rate and uses the **USB2: 16-bit stereo playback** mode.

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

Open the **Audio** page in [USB2Prefs](USB2Prefs.md) to select the
default output. **Auto** opens the first available compatible USB output.
**First** uses the lowest registered compatible output. **Custom** uses
the output chosen in the list. First and Custom fail when the chosen
output is busy. An existing allocation keeps its output until closed.

AHI recording is unsupported. Mode loading and playback on physical AmigaOS 4
hardware remain unverified.

When upgrading an installation that used the separate Bluetooth AHI mode,
remove `DEVS:AHI/bluetooth2.audio` and `DEVS:AudioModes/Bluetooth2`. Select
the USB2 mode in AHI preferences for any unit that used the old mode.
