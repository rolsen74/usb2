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
make -C Drivers/Audio -j4
make -C Drivers/Audio/Test -j4
```

Install the current USB2 stack and copy `Audio.Driver` and `Audio.Class` to
`LIBS:USB2/`. Run `AudioTest` on AmigaOS to save a one-second capture to
`RAM:usb2audio.wav`. Run `AudioTest PLAY` to play a short test tone.

See the [AHI guide](_AHI.md) for the playback driver and mode file.

Playback supports synchronous and adaptive UAC1 endpoints without feedback.
UAC2, asynchronous feedback, USB mixer controls, and AHI recording are
unsupported. The stack schedules one isochronous USB batch at a time per
endpoint, so uninterrupted streaming requires further host-controller work.
Capture and playback remain unverified on AmigaOS hardware.
