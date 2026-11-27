
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
#include <devices/usb2defaults.h>
#include <interfaces/usb2eth.h>
#include <interfaces/usb2unitidentity.h>

// --

struct USB2EthUnit
{
	const struct USB2EthOps *Ops;
	PTR Adapter;
	U32 Number;
	ULONG RegistrationID;
	U32 Opens;
	U32 Opening;
	U8 Detached;
	struct USB2EthAdapterInfo Info;
};

struct USB2EthDevice
{
	struct Library Library;
	struct USBBase *USB;
	struct SignalSemaphore OpenLock;
	struct SignalSemaphore RouteLock;
	ULONG NextRegistrationID;
	struct USB2EthUnit *Units[USB2_ETH_MAX_UNITS];
	struct USB2UnitIdentity Identity[USB2_ETH_MAX_UNITS];
	U32 Reserved[USB2_ETH_MAX_UNITS];
};

SEC_CODE struct USB2EthDevice *EthDevice_Alloc( struct USBBase *usbbase );
SEC_CODE S32 EthDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags );
SEC_CODE PTR EthDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void EthDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void EthDevice_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE S32 EthDevice_KnowsCommand( const U16 *commands, U16 command );
SEC_CODE void EthDevice_EnumAdapters( struct USB2EthDevice *base, struct IOStdReq *request );
SEC_CODE void EthDevice_Query( struct IOStdReq *request, const U16 *commands );
SEC_CODE void EthDevice_Sana2Query( struct USBBase *usbbase, struct IOSana2Req *request );
SEC_CODE struct USB2EthUnit *EthDevice_Register( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number );
SEC_CODE void EthDevice_Unregister( struct USB2EthIFace *Self, struct USB2EthUnit *unit );
SEC_CODE void EthDevice_DestroyUnit( struct ExecIFace *IExec, struct USB2EthUnit *unit );
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

SEC_CODE ULONG EthDevice_GetRegistrationID( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number );
SEC_CODE struct USB2EthUnit *EthDevice_RegisterDevice( struct USB2EthIFace *Self,
	const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type, const struct USB2_DriverMessage *msg );

#endif

// --
