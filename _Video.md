# USB Video (Experimental)

The external Video driver matches USB Video Class (UVC)
VideoStreaming interfaces (class `0x0E`, subclass `0x02`). It exposes frame
capture through `usb2video.device` and supports MJPEG and raw YUY2 over bulk
and isochronous IN endpoints. The driver selects a format and frame size
from the camera's descriptors. Applications can use `VIDEO_CMD_QUERY` to
retrieve the selected format and negotiated frame interval, then `CMD_READ`
to capture one frame.

## Build and install

```sh
make -C Drivers/Video -j4
make -C Drivers/Video/Test -j4
```

Install `Drivers/Video/bin/Video.Driver` and `Drivers/Video/Video.Class` in
`Libs:USB2` on AmigaOS 4. Run `Drivers/Video/Test/VideoTest` on AmigaOS to
capture one frame from unit 0. It saves MJPEG as `RAM:usbvideo.jpg` or raw
YUY2 as `RAM:usbvideo.yuy2`.

## Compatibility and validation

Isochronous capture requires the current stack. EHCI handles high-speed
isochronous transfers and OHCI handles full-speed isochronous transfers.
EHCI split isochronous transfers and SuperSpeed isochronous burst and Mult
modes are unsupported.

The stack, driver, and capture utility cross-compiled without warnings.
Host tests covered UVC negotiation, bulk and isochronous frame assembly,
packet-error recovery, and abort and detach cleanup. Capture with a physical
webcam on AmigaOS 4 remains unverified.
