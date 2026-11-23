
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INTERFACES_USB2ETH_H
#define INTERFACES_USB2ETH_H

#include <exec/interfaces.h>
#include <devices/sana2.h>

// --

#define USB2_ETH_DEVICE "usb2ethernet.device"
#define USB2_ETH_BACKEND_UNIT 0xffffffffUL
#define USB2_ETH_MAX_UNITS 32
#define USB2ETH_ADAPTER_REVISION 1

enum USB2EthAdapterType
{
	USB2ETH_ADAPTER_UNKNOWN,
	USB2ETH_ADAPTER_RTL8152,
	USB2ETH_ADAPTER_RTL8188,
	USB2ETH_ADAPTER_RT2870,
	USB2ETH_ADAPTER_SIS163U,
	USB2ETH_ADAPTER_RTL8821CU
};

struct USB2EthUnit;

/*
 * Open USB2_ETH_BACKEND_UNIT with an IORequest and obtain "main" v1.
 * Register assigns the lowest free number. Call Unregister after stopping
 * hardware I/O and completing queued requests. Existing opens keep their
 * old handle. Destroy releases the adapter after unregister and last close.
 * Begin must reject hardware I/O once backend shutdown starts.
 *
 * Callbacks must stay resident until Destroy and must not call back into
 * usb2ethernet.device. Open/Close manage per-opener state. Begin/Abort dispatch
 * requests with usb2ethernet.device's io_Unit still in place.
 */
struct USB2EthOps
{
	const UWORD *Commands;
	LONG ( *Open )( APTR adapter, struct IOSana2Req *request, ULONG flags );
	void ( *Close )( APTR adapter, struct IOSana2Req *request );
	void ( *Begin )( APTR adapter, struct IOSana2Req *request );
	void ( *Abort )( APTR adapter, struct IOSana2Req *request );
	void ( *Destroy )( APTR adapter );
};

struct USB2EthIFace
{
	struct InterfaceData Data;
	ULONG APICALL ( *Obtain )( struct USB2EthIFace *Self );
	ULONG APICALL ( *Release )( struct USB2EthIFace *Self );
	void APICALL ( *Expunge )( struct USB2EthIFace *Self );
	struct Interface *APICALL ( *Clone )( struct USB2EthIFace *Self );

	struct USB2EthUnit *APICALL ( *Register )( struct USB2EthIFace *Self,
		const struct USB2EthOps *ops, APTR adapter, ULONG *number );
	void APICALL ( *Unregister )( struct USB2EthIFace *Self, struct USB2EthUnit *unit );

	/* These vectors require usb2ethernet.device revision USB2ETH_ADAPTER_REVISION. */
	struct USB2EthUnit *APICALL ( *RegisterAdapter )( struct USB2EthIFace *Self,
		const struct USB2EthOps *ops, APTR adapter, ULONG *number, ULONG type );
	/* Pass a live open request to identify its adapter, or NULL to look up a unit number. */
	ULONG APICALL ( *GetAdapter )( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number );
};

#endif
