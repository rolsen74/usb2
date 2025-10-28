
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

static PTR _manager_Init( PTR Dummy UNUSED, PTR SegList, struct ExecBase *mySysBase )
{
struct EthernetBase *ethBase;

	ethBase	= NULL;

	SysBase = (PTR) mySysBase;
	IExec = (PTR) mySysBase->MainInterface;

	USBDEBUG( "RTL8188 : _manager_Init : devNane = %s", DEVNAME );

	// Make sure we havent started
	if ( FindName( & mySysBase->DeviceList, DEVNAME ))
	{
		USBDEBUG( "_manager_Init : Device allready loaded" );
		goto bailout;
	}

	extern const PTR devInterfaces[];
	ethBase = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct EthernetBase ),
		CLT_Interfaces,	devInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! ethBase )
	{
		USBDEBUG( "_manager_Init : Failed to create device" );
		goto bailout;
	}

	ethBase->eth_Base.dd_Library.lib_Node.ln_Type	= NT_DEVICE;
	ethBase->eth_Base.dd_Library.lib_Node.ln_Pri	= 0;
	ethBase->eth_Base.dd_Library.lib_Node.ln_Name	= DEVNAME;
	ethBase->eth_Base.dd_Library.lib_Flags			= LIBF_SUMUSED|LIBF_CHANGED;
	ethBase->eth_Base.dd_Library.lib_Version		= VERSION;
	ethBase->eth_Base.dd_Library.lib_Revision		= REVISION;
	ethBase->eth_Base.dd_Library.lib_IdString		= VSTRING;
	ethBase->eth_SegList = SegList;

	if ( Resources_Init( ethBase ))
	{
		USBDEBUG( "RTL8188 : Opening Resources...  Success" );

		AddDevice( (PTR) ethBase );

		DebugPrintF( "RTL8188 : Rock'n Roll\n" );
	}
	else
	{
		USBERROR( "RTL8188 : Opening Resources...  Failed" );

		Resources_Free( ethBase );

		ethBase = NULL;
	}

bailout:

	return( NULL );
}

// --
