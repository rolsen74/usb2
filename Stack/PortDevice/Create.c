
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"
#include "usb2_rev.h"

// --

SEC_CODE static U32
Obtain ( struct Interface * Self )
{
	return ( ++Self->Data.RefCount );
}

// --

SEC_CODE static U32
Release ( struct Interface * Self )
{
	return ( --Self->Data.RefCount );
}

// --

SEC_CODE static PTR
Expunge ( struct DeviceManagerInterface * Self UNUSED )
{
	return ( NULL );
}

// --

SEC_RODATA static const PTR ManagerVectors[] = //
	{ (PTR)Obtain,
	  (PTR)Release,
	  NULL,
	  NULL,
	  (PTR)PortDevice_Open,
	  (PTR)PortDevice_Close,
	  (PTR)Expunge,
	  NULL,
	  (PTR)PortDevice_BeginIO,
	  (PTR)PortDevice_AbortIO,
	  (PTR)-1 };

SEC_RODATA static const struct TagItem ManagerTags[] = //
	{
		{ MIT_Name, ( Tag ) "__device" },		  //
		{ MIT_VectorTable, (Tag)ManagerVectors }, //
		{ MIT_Version, 1 },						  //
		{ TAG_END, 0 }							  //
	};

SEC_RODATA static const PTR MainVectors[] = //
	{
		(PTR)Obtain,				//
		(PTR)Release,				//
		NULL,						//
		NULL,						//
		(PTR)PortDevice_Register,	//
		(PTR)PortDevice_Unregister, //
		(PTR)-1						//
	};

SEC_RODATA static const struct TagItem MainTags[] = //
	{
		{ MIT_Name, ( Tag ) "main" },		   //
		{ MIT_VectorTable, (Tag)MainVectors }, //
		{ MIT_Version, 1 },					   //
		{ TAG_END, 0 }						   //
	};

SEC_RODATA static const PTR Interfaces[] = //
	{
		(PTR)ManagerTags, //
		(PTR)MainTags,	  //
		NULL			  //
	};

// --

SEC_CODE static struct USB2PortDevice *
Alloc ( struct ExecIFace * IExec, STR name, STR id, U8 serial )
{
	struct USB2PortDevice * base;

	base = (PTR)IExec->CreateLibraryTags (				//
		CLT_NoLegacyIFace, TRUE,						//
		CLT_Interfaces, Interfaces,						//
		CLT_DataSize, sizeof ( struct USB2PortDevice ), //
		TAG_END											//
	);

	if ( ! base )
	{
		return ( NULL );
	}

	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Name = name;
	base->Library.lib_Flags		   = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version	   = 1;
	base->Library.lib_IdString	   = id;
	base->IExec					   = IExec;
	base->Serial				   = serial;

	IExec->InitSemaphore ( &base->Lock );

	return ( base );
}

// --

SEC_CODE void
PortDevice_Create ( struct USBBase * usbbase )
{
	struct USB2PortDevice * serial;
	struct USB2PortDevice * parallel;
	struct ExecIFace *		IExec;

	IExec = usbbase->usb_IExec;

	// --
	// Serial

	serial = Alloc ( IExec, USB2_SERIAL_DEVICE, USB2_SERIAL_DEVICE " 1.0 (" DATE ")\r\n", TRUE );

	if ( ! serial )
	{
		USBERROR ( "Could not allocate USB serial device" );
		return;
	}

	// --
	// Parallel

	parallel = Alloc ( IExec, USB2_PARALLEL_DEVICE, USB2_PARALLEL_DEVICE " 1.0 (" DATE ")\r\n", FALSE );

	if ( ! parallel )
	{
		IExec->DeleteLibrary ( (PTR)serial );

		USBERROR ( "Could not allocate USB parallel device" );
		return;
	}

	// --
	// The permanent port devices have vectors in this Kickstart module.
	usbbase->usb_Library.lib_OpenCnt++;

	IExec->AddDevice ( (PTR)serial );

	IExec->AddDevice ( (PTR)parallel );
}

// --
