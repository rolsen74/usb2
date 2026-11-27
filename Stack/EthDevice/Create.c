
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"
#include "usb2_rev.h"

// --

SEC_CODE static U32
Obtain( struct Interface *Self )
{
	return ( ++Self->Data.RefCount );
}

// --

SEC_CODE static U32
Release( struct Interface *Self )
{
	return ( --Self->Data.RefCount );
}

// --

SEC_CODE static PTR
Expunge( struct DeviceManagerInterface *Self UNUSED )
{
	return ( NULL );
}

// --

SEC_RODATA static const PTR ManagerVectors[] =
{
	(PTR) Obtain,
	(PTR) Release,
	NULL,
	NULL,
	(PTR) EthDevice_Open,
	(PTR) EthDevice_Close,
	(PTR) Expunge,
	NULL,
	(PTR) EthDevice_BeginIO,
	(PTR) EthDevice_AbortIO,
	(PTR) -1
};

SEC_RODATA static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__device" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR MainVectors[] =
{
	(PTR) Obtain,
	(PTR) Release,
	NULL,
	NULL,
	(PTR) EthDevice_Register,
	(PTR) EthDevice_Unregister,
	(PTR) EthDevice_RegisterAdapter,
	(PTR) EthDevice_GetAdapter,
	(PTR) EthDevice_RegisterNamedAdapter,
	(PTR) EthDevice_GetAdapterTitle,
	(PTR) EthDevice_RegisterClassAdapter,
	(PTR) EthDevice_GetAdapterInfo,
	(PTR) EthDevice_GetRegistrationID,
	(PTR) EthDevice_RegisterDevice,
	(PTR) -1
};

SEC_RODATA static const struct TagItem MainTags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) MainVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR Interfaces[] = { (PTR) ManagerTags, (PTR) MainTags, NULL };

// --

SEC_CODE struct USB2EthDevice *
EthDevice_Alloc( struct USBBase *usbbase )
{
	struct USB2EthDevice *base;
	struct ExecIFace *IExec;

	IExec = usbbase->usb_IExec;

	base = (PTR) IExec->CreateLibraryTags(
		CLT_NoLegacyIFace, TRUE,
		CLT_Interfaces, Interfaces,
		CLT_DataSize, sizeof( *base ),
		TAG_END );

	if ( ! base )
	{
		USBERROR( "Could not allocate USB Ethernet device" );

		return ( NULL );
	}

	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Name = USB2_ETH_DEVICE;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 1;
	base->Library.lib_Revision = USB2ETH_RECONNECT_REVISION;
	base->Library.lib_IdString = USB2_ETH_DEVICE " 1.12 (" DATE ")\r\n";
	base->USB = usbbase;

	IExec->InitSemaphore( & base->OpenLock );

	IExec->InitSemaphore( & base->RouteLock );

	return ( base );
}

// --
