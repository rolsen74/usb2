
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static PTR _manager_Expunge( struct DeviceManagerInterface *Self )
{
struct EthernetBase *ethBase;
PTR seglist;

	USBDEBUG( "RTL8188 : _manager_Expunge" );

	ethBase = (PTR) Self->Data.LibBase;

	if ( ! ethBase->eth_Base.dd_Library.lib_OpenCnt )
	{
		seglist = ethBase->eth_SegList;

		Resources_Free( ethBase );

		Remove( (PTR) ethBase );

		DeleteLibrary( (PTR) ethBase );
		ethBase = NULL;
	}
	else
	{
		ethBase->eth_Base.dd_Library.lib_Flags |= LIBF_DELEXP;
		seglist = NULL;
	}

	return( seglist );
}

// --
