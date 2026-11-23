
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_ETHDEVICE_H
#define USB2_ETHDEVICE_H

#include "usb2_all.h"
#include <devices/newstyle.h>
#include <interfaces/usb2eth.h>

// --

struct USB2EthUnit
{
	const struct USB2EthOps *Ops;
	PTR Adapter;
	U32 Number;
	U32 Opens;
	U8 Detached;
	struct USB2EthAdapterInfo Info;
};

struct USB2EthDevice
{
	struct Library Library;
	struct USBBase *USB;
	struct SignalSemaphore OpenLock;
	struct SignalSemaphore RouteLock;
	struct USB2EthUnit *Units[USB2_ETH_MAX_UNITS];
};

SEC_CODE struct USB2EthDevice *EthDevice_Alloc( struct USBBase *usbbase );
SEC_CODE S32 EthDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags );
SEC_CODE PTR EthDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void EthDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void EthDevice_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void EthDevice_Query( struct IOStdReq *request, const U16 *commands );
SEC_CODE struct USB2EthUnit *EthDevice_Register( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number );
SEC_CODE void EthDevice_Unregister( struct USB2EthIFace *Self, struct USB2EthUnit *unit );
SEC_CODE struct USB2EthUnit *EthDevice_RegisterAdapter( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type );
SEC_CODE ULONG EthDevice_GetAdapter( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number );
SEC_CODE struct USB2EthUnit *EthDevice_RegisterNamedAdapter( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type, const char *title );
SEC_CODE ULONG EthDevice_GetAdapterTitle( struct USB2EthIFace *Self, ULONG number, char *title );
SEC_CODE struct USB2EthUnit *EthDevice_RegisterClassAdapter( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type,
	const char *title, const char *driver, const char *alias );
SEC_CODE LONG EthDevice_GetAdapterInfo( struct USB2EthIFace *Self, const struct IORequest *request,
	struct USB2EthAdapterInfo *info );

#endif

// --
