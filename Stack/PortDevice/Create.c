
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

SEC_RODATA static const PTR SerialVectors[] =
{
	(PTR)Obtain,
	(PTR)Release,
	NULL,
	NULL,
	(PTR)PortDevice_SetParams,
	(PTR)-1
};

SEC_RODATA static const struct TagItem SerialTags[] =
{
	{ MIT_Name, (Tag)"serial" },
	{ MIT_VectorTable, (Tag)SerialVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR SerialInterfaces[] = { (PTR)ManagerTags, (PTR)MainTags, (PTR)SerialTags, NULL };

// --

SEC_RODATA static const PTR PrinterVectors[] =
{
	(PTR) Obtain,
	(PTR) Release,
	NULL,
	NULL,
	(PTR) PortDevice_Register,
	(PTR) PortDevice_Unregister,
	(PTR) PortDevice_JobClosing,
	(PTR) PortDevice_FinishJob,
	(PTR) PortDevice_Quiesce,
	(PTR) -1
};

SEC_RODATA static const struct TagItem PrinterTags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) PrinterVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR PrinterInterfaces[] = { (PTR) ManagerTags, (PTR) PrinterTags, NULL };

// --

SEC_CODE static struct USB2PortDevice *
Alloc ( struct USBBase * usbbase, STR name, STR id, U8 revision, enum USB2PortType type )
{
	struct USB2PortDevice * base;
	struct ExecIFace * IExec;
	const PTR *interfaces;

	IExec = usbbase->usb_IExec;
	interfaces = Interfaces;

	if ( type == PORT_SERIAL )
	{
		interfaces = SerialInterfaces;
	}
	else if ( type == PORT_PRINTER )
	{
		interfaces = PrinterInterfaces;
	}

	base = (PTR)IExec->CreateLibraryTags (				//
		CLT_NoLegacyIFace, TRUE,						//
		CLT_Interfaces, interfaces, //
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
	base->Library.lib_Revision	   = revision;
	base->Library.lib_IdString	   = id;
	base->IExec					   = IExec;
	base->USB					   = usbbase;
	base->Type					   = type;

	IExec->InitSemaphore ( &base->Lock );

	return ( base );
}

// --

SEC_CODE S32
PortDevice_Create ( struct USBBase * usbbase )
{
	struct USB2PortDevice * serial;
	struct USB2PortDevice * parallel;
	struct USB2PortDevice * printer;
	struct ExecIFace *		IExec;

	IExec = usbbase->usb_IExec;

	// --
	// Serial

	serial = Alloc ( usbbase, USB2_SERIAL_DEVICE, USB2_SERIAL_DEVICE " 1.4 (" DATE ")\r\n", 4, PORT_SERIAL );

	if ( ! serial )
	{
		USBERROR ( "Could not allocate USB serial device" );

		return ( FALSE );
	}

	// --
	// Parallel

	parallel = Alloc ( usbbase, USB2_PARALLEL_DEVICE, USB2_PARALLEL_DEVICE " 1.4 (" DATE ")\r\n", 4, PORT_PARALLEL );

	if ( ! parallel )
	{
		IExec->DeleteLibrary ( (PTR)serial );

		USBERROR ( "Could not allocate USB parallel device" );

		return ( FALSE );
	}

	printer = Alloc ( usbbase, USB2_PRINTER_DEVICE, USB2_PRINTER_DEVICE " 1.8 (" DATE ")\r\n", 8, PORT_PRINTER );

	if ( ! printer )
	{
		IExec->DeleteLibrary ( (PTR)parallel );

		IExec->DeleteLibrary ( (PTR)serial );

		USBERROR ( "Could not allocate USB printer device" );

		return ( FALSE );
	}

	// The permanent port devices have vectors in this Kickstart module.
	usbbase->usb_Library.lib_OpenCnt++;

	IExec->AddDevice ( (PTR)serial );

	IExec->AddDevice ( (PTR)parallel );

	IExec->AddDevice ( (PTR)printer );

	return ( TRUE );
}

// --
