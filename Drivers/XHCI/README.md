# xHCI driver status

The xHCI implementation is split by operation, following the UHCI, OHCI, and EHCI layouts. It has controller reset/start/stop, 32-bit physical DMA rings, the primary event ring, scratchpads, root port operations, slot addressing, endpoint configuration, and control, bulk, interrupt, and basic isochronous transfer paths.

The default build enables xHCI (`DRV_XHCI=1`). Build it with `make -B -j4`, or set `DRV_XHCI=0` to exclude it while testing other controllers.

Current hardware scope:

- Root port devices and downstream hub children have slot routing and USB2 transaction translator context setup. Downstream hub enumeration has not been checked on hardware.
- The stack does not retain SuperSpeed Endpoint Companion descriptors, so endpoints using bursts, streams, or larger ESIT payloads are not configured yet.
- Isochronous transfers use one scheduled packet per TD and need hardware testing, especially at SuperSpeed.
- The controller paths have passed the PPC cross compiler, but have not been exercised against a physical xHCI controller.

Start hardware testing with one directly connected USB2 device and capture the xHCI initialization, port change, slot command, and first descriptor transfer results. Keep the existing UHCI/OHCI/EHCI drivers available while testing.
