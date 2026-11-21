# USB Audio (Experimental)

The Audio driver exposes `usb2audio.device` for USB Audio
Class 1.0 mono and stereo PCM16. Capture units start at 0 and playback units
start at 16. Use `AUDIO_CMD_QUERY` to get the selected format. `CMD_READ`
and `CMD_WRITE` transfer interleaved little-endian PCM in whole sample frames.
The driver selects the first usable alternate setting and checks the rate
against its USB service interval and packet capacity. With no read or write
active, `AUDIO_CMD_SET_RATE` takes a native `U32` rate in Hz supported by
the selected alternate setting. `AUDIO_CMD_CHECK_RATE` validates the
same rate without changing it. For supported microphones,
`AUDIO_CMD_GET_GAIN` reports the gain range and current value, and
`AUDIO_CMD_SET_GAIN` takes a native `S32` value in units of 1/256 dB.

## Build and install

```sh
make -C Drivers/Audio
make -C Drivers/Audio/Test
```

Install the current USB2 stack and copy `Audio.Driver` and `Audio.Class` to
`LIBS:USB2/`. Reboot after installing `Audio.Class` so the USB2 stack
loads the class rule at startup. The archive places `USB2AudioTest` in
`Tools/`. Stop the webcam preview before testing the microphone on the same
EHCI controller.
Run `USB2AudioTest` on AmigaOS and speak while it records. It saves one
second to `RAM:usb2audio.wav` and prints the peak sample level. Capture
times out after ten seconds and can be interrupted with Ctrl+C. Run
`USB2AudioTest PLAY` to play a short test tone on a supported USB output
or `USB2AudioTest PLAY RAM:usb2audio.wav` to play back the recording.

File playback accepts mono or stereo PCM16 WAV and converts it to the output
unit's sample rate and channel count. Both the file and converted samples
must fit in their respective 32 MiB buffers.
If capture unit 0 will not open, `USB2AudioTest CHECK` reports the driver
paths, the loaded driver version, and whether the audio device is registered.

See the [AHI guide](AHI.md) for the playback driver and mode file.

## Playback through EHCI

Playback supports synchronous and adaptive UAC1 endpoints without feedback.
Full-speed devices with an unspecified synchronization type can also use
playback when they advertise one discrete rate and a 1 ms packet sized
exactly for that rate.
Full-speed USB audio can use OHCI or an EHCI controller through a high-speed
hub. Playback error `-18` means the controller rejected the USB speed for
the transfer. Use USB2Analyzer to check the controller and the hub path.
Asynchronous playback feedback, general USB mixer controls, and AHI recording
are unsupported.

For full-speed playback through EHCI, the driver prefers a supported rate
that fits within one 188-byte start-split when the initial rate needs
multiple start-splits. It falls back to the initial rate if the device
rejects the preferred rate. EHCI can schedule two playback batches
in succession on the same endpoint. A missed isochronous slot can still
produce a short write (`-43`). The completed byte count excludes missed
samples.

## USB Audio Class 2.0

Audio.Driver 1.2 adds experimental UAC2 capture and playback for mono and
stereo PCM16. The driver uses the current rate reported by the stream's
directly connected Clock Source. Playback requires a synchronous or adaptive
endpoint without feedback. Clock selectors, rate switching, UAC2 gain
controls, and wider PCM formats are outside this initial support.

Install both the updated `Audio.Class` and `Audio.Driver` and reboot.
Use `USB2AudioTest PLAY` for playback or `USB2AudioTest` for capture.
UAC2 has passed descriptor tests and awaits hardware testing.

## Hardware validation

Microphone capture was verified on an AmigaOne X5000/20 with a Logitech
`046D:0826` webcam. After programming the endpoint's fixed rate at the
start of recording, `USB2AudioTest` saved a one-second, 16 kHz mono PCM16
WAV with a peak sample level of 4,170 out of 32,768. WAV playback was
verified on the same machine with a C-Media `0D8C:0014` adapter through EHCI.
During live monitoring, 44.1 kHz stereo PCM16 used one split per 1 ms USB
frame and had far fewer short writes than 48 kHz. The remaining logged
misses each dropped one packet. Use the rate chooser in
[USB2VideoTest](Video.md) to compare both rates.
