
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static PTR _manager_Init( PTR Dummy UNUSED, PTR SegList, struct ExecBase *mySysBase )
{
struct PrinterBase *devBase;

	devBase	= NULL;

	SysBase = (PTR) mySysBase;
	IExec = (PTR) mySysBase->MainInterface;

	USBDEBUG( "Printer : _manager_Init" );

	// Make sure we havent started
	if ( FindName( & mySysBase->DeviceList, DEVNAME ))
	{
		USBDEBUG( "_manager_Init : Device allready loaded" );
		goto bailout;
	}

	extern const PTR devInterfaces[];
	devBase = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct PrinterBase ),
		CLT_Interfaces,	devInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! devBase )
	{
		USBDEBUG( "_manager_Init : Failed to create device" );
		goto bailout;
	}

	devBase->dev_Base.dd_Library.lib_Node.ln_Type	= NT_DEVICE;
	devBase->dev_Base.dd_Library.lib_Node.ln_Pri	= 0;
	devBase->dev_Base.dd_Library.lib_Node.ln_Name	= DEVNAME;
	devBase->dev_Base.dd_Library.lib_Flags			= LIBF_SUMUSED|LIBF_CHANGED;
	devBase->dev_Base.dd_Library.lib_Version		= VERSION;
	devBase->dev_Base.dd_Library.lib_Revision		= REVISION;
	devBase->dev_Base.dd_Library.lib_IdString		= VSTRING;
	devBase->dev_SegList = SegList;

	if ( Resources_Init( devBase ))
	{
		USBDEBUG( "Printer : Opening Resources...  Success" );

		AddDevice( (PTR) devBase );

		DebugPrintF( "Printer : Rock'n Roll\n" );
	}
	else
	{
		USBERROR( "Printer : Opening Resources...  Failed" );

		Resources_Free( devBase );

		devBase = NULL;
	}

bailout:

	return( devBase );
}

// --
