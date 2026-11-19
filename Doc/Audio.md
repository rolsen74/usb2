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
`LIBS:USB2/`. The archive places `USB2AudioTest` in `Tools/`. Stop the
webcam preview before testing the microphone on the same EHCI controller.
Run `USB2AudioTest` on AmigaOS and speak while it records. It saves one
second to `RAM:usb2audio.wav` and prints the peak sample level. Capture
times out after ten seconds and can be interrupted with Ctrl+C. Run
`USB2AudioTest PLAY` to play a short test tone on a supported output.

See the [AHI guide](AHI.md) for the playback driver and mode file.

Playback supports synchronous and adaptive UAC1 endpoints without feedback.
UAC2, asynchronous feedback, USB mixer controls, and AHI recording are
unsupported. The stack schedules one isochronous USB batch at a time per
endpoint, so uninterrupted streaming requires further host-controller work.
Capture and playback remain unverified on AmigaOS hardware.
