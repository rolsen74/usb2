
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Manager.h"

// --

	struct Library *SysBase;
	struct ExecIFace *IExec;

// --

PTR
_manager_Init( PTR Dummy UNUSED, PTR SegList, struct ExecBase *mySysBase )
{
	struct EthernetBase *ethBase;

	ethBase	= NULL;

	SysBase = (PTR) mySysBase;
	IExec = (PTR) mySysBase->MainInterface;

	USBDEBUG( "RT2870 : _manager_Init : device name = %s", DEVNAME );

	// Make sure we havent started
	if ( FindName( & mySysBase->DeviceList, DEVNAME ))
	{
		USBDEBUG( "_manager_Init : Device allready loaded" );
		goto bailout;
	}

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
		USBDEBUG( "RT2870 : Opening Resources...  Success" );

		AddDevice( (PTR) ethBase );

		USBINFO( "RT2870: " VERS " registered" );
	}
	else
	{
		USBERROR( "RT2870 : Opening Resources...  Failed" );

		Resources_Free( ethBase );

		DeleteLibrary( (PTR) ethBase );

		ethBase = NULL;
	}

bailout:

	return ( ethBase );
}

// --
