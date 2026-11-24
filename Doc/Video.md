# USB Video (Experimental)

The external Video driver matches USB Video Class (UVC)
VideoStreaming interfaces (class `0x0E`, subclass `0x02`). It exposes frame
capture through `usb2video.device` and supports MJPEG and raw YUY2 over bulk
and isochronous IN endpoints. The driver prefers MJPEG when the camera offers
it and selects the first frame size by default. Applications can use
`VIDEO_CMD_GET_MODE` to enumerate the camera's supported capture settings and
`VIDEO_CMD_SET_MODE` to request one while capture is stopped.
`VIDEO_CMD_QUERY` reports the negotiated format and interval, and `CMD_READ`
captures one frame. Use `VIDEO_CMD_STOP` to discard buffered packets when
pausing capture.

## Build and install

```sh
make -C Drivers/Video
make -C Drivers/Video/Test
```

Install `Drivers/Video/bin/cls_Video.Driver` and `Drivers/Video/cls_Video.Class` in
`Libs:USB2` on AmigaOS 4. Run `USB2VideoTest` to open the settings window
with **Video Capture**, **Audio Capture**, and **Audio Output** groups.
Video Capture is checked when a camera is available. If multiple video units
are available, use the **Camera** selector to update the modes and default
audio input in the same window. A camera with one format still shows that
format as an active cycle choice.
From a Shell, use `USB2VideoTest CAMERAS` to list available video units and
`USB2VideoTest UNIT 1` to select unit 1 at launch.
The distribution archive places the tool in `Tools/`. Click **Start** to
hide settings and start the checked groups. With Video Capture unchecked,
an audio session window opens and the camera stream stays idle. Close the
session window to stop and show the same settings window with your
selections and its original position. The video
preview passes MJPEG frames from memory to the installed JPEG datatype and
converts YUY2 frames to RGB for display.

The preview is centered in a 640 × 480 area when the screen has room.
The ReAction settings window groups the controls under **Video Capture**,
**Audio Capture**, and **Audio Output**. Change mode to refresh the sizes
and rates for that format.
The size menu shows **Default** above a separator, then the remaining sizes
ordered by width and height. FPS choices run from lowest to highest. Changing
size refreshes the rates for that size. Mode and size changes keep the
previous values when offered and otherwise choose the closest available
values. The driver starts with the camera's default frame and interval for
the preferred format when the camera accepts them. If that probe fails,
**Default** uses the first mode negotiated successfully at attachment.

**Audio Capture** enables the input selector. Check **Audio Output** to
monitor the selected input through USB audio or AHI's configured unit 0.
The **Default rate** chooser in **Audio Output** also offers 44100 Hz and
48000 Hz when the selected USB output supports them. The selected rate
applies when monitoring starts.
**Mic Gain** sets the selected USB microphone's input gain when it exposes
a supported gain control. It shows the current level within the device's
range and applies changes when capture starts.
The capture window has a **Volume** slider for monitoring. It starts at
70% and adjusts playback without changing the WAV recording level.
Check **Save WAV** to enable the file picker and record to the selected
destination. The default is `RAM:usb2video-audio.wav`. The tool prefers a
USB audio input with the camera's vendor and product IDs.
Audio choices are detected when settings first opens and require the
[USB audio driver](Audio.md).

Run `USB2VideoTest TRACE` to log shutdown steps to the serial debug output.

For video recording, check **Save AVI** in the **Video Capture** group and
choose a destination with its file picker. The default is
`RAM:usb2video.avi`. The AVI contains the captured MJPEG or YUY2 frames.
Playback uses the average captured frame rate when the timer is available.
When **Save WAV** is also checked, audio goes to a separate WAV. Both files
are finalized when you close the session window. AVI capture continues if only
the preview decoder fails. AVI recording stops at the 2 GiB limit. If the
requested filename is occupied or publication fails, the tool keeps the
finalized recording separately and displays its path.

Live video, recording and monitoring can share an EHCI controller when the
streams fit within its [periodic schedule](EHCI.md).
Monitoring buffers two audio blocks before playback starts. If a USB
output write is short, monitoring can continue with the next block.
Recording continues if the monitoring output stops.

Run `USB2VideoTest MODES` in a Shell to list unit 0's resolutions and frame
rates. Launch at a listed setting with, for example, `USB2VideoTest MODE 4`.
Use `UNIT` with either command to choose a different camera.
The preview title shows the measured display rate and
the negotiated camera frame rate while capture is running.
For a continuous range, the list includes the default and the two endpoints.
Use `MODE 0` to restore the default. The selected mode stays active until
another selection or the next reconnect.

Run `USB2VideoTest SAVE` from a Shell to capture 30 frames and save the last
one as `RAM:usbvideo.jpg` (MJPEG) or `RAM:usbvideo.yuy2` (raw YUY2).
Combine `MODE` and `SAVE` to capture in a selected mode.
The driver allows ten seconds for an active read to produce a complete
frame. A timed-out request reports zero frame bytes.
If the tool cannot open a video unit, check that `cls_Video.Class` and `cls_Video.Driver`
were installed before connecting the webcam, then reconnect it.

## Compatibility and validation

Isochronous capture requires the current stack. EHCI handles high-speed
isochronous transfers and full-speed transfers through a high-speed hub.
OHCI also handles full-speed isochronous transfers.
The driver discards incomplete YUY2 frames while waiting for a complete
one. The first few discarded frames are reported on the serial console
with their byte counts.
Use the updated `usb2.device` when the webcam has an Interface Association
Descriptor; older builds may not bind its VideoStreaming interface.
See the [EHCI guide](EHCI.md) for split-transfer limits. SuperSpeed
isochronous burst and Mult modes are unsupported.

Video uses two USB read buffers while capturing. For high-speed isochronous
input, EHCI can schedule the second batch immediately after the first.
The video driver keeps unused packets from a completed batch for the next frame.
After a known gap in the USB reads, it discards the partial frame and waits for
a frame boundary.
Stopping capture releases the USB read buffers and returns the isochronous
interface to alternate setting 0. The selected mode is kept for the next start.
Mode changes probe the camera while it is idle. Capture commits the negotiated
mode before enabling streaming.

The stack, driver, and capture utility cross-compiled without warnings.
Host tests covered UVC negotiation, bulk and isochronous frame assembly,
packet-error recovery, and abort and detach cleanup. On an AmigaOne X5000/20,
a `046D:0826` webcam registered as unit 0 and saved a 9,973-byte MJPEG frame
at 640 x 480 with frame interval 333333. YUY2 at the same size reported UVC
payload errors and timed out. The saved JPEG opened successfully, but its
first captured image was nearly black. After increasing the isochronous
read size from 16 to 256 packets on this camera, a later run completed at
least three MJPEG frames (35,225, 49,583, and 49,906 bytes). The saved
55,714-byte JPEG from that run decoded as a clear 640 x 480 image.

On the same Amiga, the preview reached the camera's 30 fps limit at 320 x 240
after the EHCI periodic scheduling update. The previous build displayed
13–16 fps at the same resolution. With webcam audio monitored through a
full-speed USB output, a later test displayed about 24–25 fps and played
audible webcam audio with some static. The C-Media `0D8C:0014` output
reported far fewer short writes at 44100 Hz than at 48000 Hz. A few
single-packet misses remained at 44100 Hz.
