# USB Audio (Experimental)

The Audio driver exposes `usb2audio.device` for USB Audio
Class 1.0 mono and stereo PCM16. Capture units start at 0 and playback units
start at 16. Use `AUDIO_CMD_QUERY` to get the selected format. `CMD_READ`
and `CMD_WRITE` transfer interleaved little-endian PCM in whole sample frames.
The driver selects the first usable alternate setting and checks the rate
against its USB service interval and packet capacity. With no read or write
active, `AUDIO_CMD_SET_RATE` takes a native `U32` rate in Hz supported by
the selected alternate setting.

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

Playback supports synchronous and adaptive UAC1 endpoints without feedback.
Full-speed USB audio can use OHCI or an EHCI controller through a high-speed
hub. Playback error `-18` means the controller rejected the USB speed for
the transfer. Use USB2Analyzer to check the controller and the hub path.
UAC2, asynchronous feedback, USB mixer controls, and AHI recording are
unsupported. The stack schedules one isochronous USB batch at a time per
endpoint, so uninterrupted streaming requires further host-controller work.
Microphone capture was verified on an AmigaOne X5000/20 with a Logitech
`046D:0826` webcam. After programming the endpoint's fixed rate at the
start of recording, `USB2AudioTest` saved a one-second, 16 kHz mono PCM16
WAV with a peak sample level of 4,170 out of 32,768. WAV playback was
verified on the same machine with a C-Media `0D8C:0014` adapter through EHCI.
