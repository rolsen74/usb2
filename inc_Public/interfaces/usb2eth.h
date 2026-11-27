
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
#define USB2ETH_TITLE_REVISION 6
#define USB2ETH_TITLE_SIZE 96
#define USB2ETH_CLASS_REVISION 7
#define USB2ETH_REGISTRATION_REVISION 8
#define USB2ETH_RECONNECT_REVISION 12

/* Values used by existing driver and client pairs. The stack treats type as opaque. */
enum USB2EthAdapterType
{
	USB2ETH_ADAPTER_UNKNOWN,
	USB2ETH_ADAPTER_RTL8152,
	USB2ETH_ADAPTER_RTL8188,
	USB2ETH_ADAPTER_RT2870,
	USB2ETH_ADAPTER_SIS163U,
	USB2ETH_ADAPTER_RTL8821CU,
	USB2ETH_ADAPTER_RTL8811AU,
	USB2ETH_ADAPTER_AX88772
};

struct USB2EthUnit;
struct USB2_DriverMessage;

struct USB2EthAdapterInfo
{
	ULONG Type;
	char Title[USB2ETH_TITLE_SIZE];
	char Driver[USB2ETH_TITLE_SIZE];
	char Alias[USB2ETH_TITLE_SIZE];
};

#define USB2_ETH_CMD_ENUM_ADAPTERS 0x8011
#define USB2_ETH_ADAPTER_LIST_VERSION 1 // io_Offset

struct USB2EthAdapterEntry
{
	ULONG Number;
	struct USB2EthAdapterInfo Info;
};

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

	/* These vectors require USB2ETH_TITLE_REVISION. title is copied on registration. */
	struct USB2EthUnit *APICALL ( *RegisterNamedAdapter )( struct USB2EthIFace *Self,
		const struct USB2EthOps *ops, APTR adapter, ULONG *number, ULONG type, const char *title );
	/* Provide a title buffer of USB2ETH_TITLE_SIZE bytes. Returns the current adapter type. */
	ULONG APICALL ( *GetAdapterTitle )( struct USB2EthIFace *Self, ULONG number, char *title );

	/* These vectors require USB2ETH_CLASS_REVISION. Metadata strings are optional and copied. */
	struct USB2EthUnit *APICALL ( *RegisterClassAdapter )( struct USB2EthIFace *Self,
		const struct USB2EthOps *ops, APTR adapter, ULONG *number, ULONG type,
		const char *title, const char *driver, const char *alias );
	/* Copy metadata for a live open, even for registered adapters with type UNKNOWN. */
	LONG APICALL ( *GetAdapterInfo )( struct USB2EthIFace *Self, const struct IORequest *request,
		struct USB2EthAdapterInfo *info );
	/* Since USB2ETH_REGISTRATION_REVISION. Zero means no live registration. */
	ULONG APICALL ( *GetRegistrationID )( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number );
	/* Since USB2ETH_RECONNECT_REVISION. Match the unit number without carrying over SANA-II opens. */
	struct USB2EthUnit *APICALL ( *RegisterDevice )( struct USB2EthIFace *Self,
		const struct USB2EthOps *ops, APTR adapter, ULONG *number, ULONG type, const struct USB2_DriverMessage *msg );
};

#endif
