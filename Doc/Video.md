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

Install `Drivers/Video/bin/Video.Driver` and `Drivers/Video/Video.Class` in
`Libs:USB2` on AmigaOS 4. Run `USB2VideoTest` to open the preview for unit 0.
The distribution archive places it in `Tools/`. Click **Start / Stop** to
toggle capture. The preview passes MJPEG frames from memory to the installed
JPEG datatype and converts YUY2 frames to RGB for display.
Stopping capture leaves the last frame visible.

The preview is centered in a 640 × 480 area when the screen has room.
The controls are **Start / Stop**, mode, size, and FPS, in that order. With
capture stopped, change mode to refresh the sizes and rates for that format.
Changing size refreshes the rates for that size. The selector keeps the
previous size and frame interval when offered and otherwise chooses the
closest available values. These controls require the updated `Video.Driver`.

Run `USB2VideoTest MODES` in a Shell to list the camera's resolutions and frame
rates. Launch at a listed setting with, for example, `USB2VideoTest MODE 4`.
The preview title shows the measured display rate and
the negotiated camera frame rate while capture is running.
For a continuous range, the list includes the default and the two endpoints.
Use `MODE 0` to restore the default. The selected mode stays active until
another selection or the next reconnect.

Run `USB2VideoTest SAVE` from a Shell to capture 30 frames and save the last
one as `RAM:usbvideo.jpg` (MJPEG) or `RAM:usbvideo.yuy2` (raw YUY2).
Combine `MODE` and `SAVE` to capture in a selected mode.
Each frame read times out after ten seconds.
If the tool cannot open unit 0, check that `Video.Class` and `Video.Driver`
were installed before connecting the webcam, then reconnect it.

## Compatibility and validation

Isochronous capture requires the current stack. EHCI handles high-speed
isochronous transfers and full-speed transfers through a high-speed hub.
OHCI also handles full-speed isochronous transfers.
Use the updated `usb2.device` when the webcam has an Interface Association
Descriptor; older builds may not bind its VideoStreaming interface.
See the [EHCI guide](EHCI.md) for split-transfer limits. SuperSpeed
isochronous burst and Mult modes are unsupported.

Video submits three USB reads while capturing. The stack starts queued reads
in order, and the driver keeps packets from the current USB read for the next
video frame. EHCI still schedules one read at a time.

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

On the same Amiga, the preview displayed 13–16 fps at 320 x 240 with
three USB reads queued. An earlier run at 320 x 176 displayed about 12 fps.
