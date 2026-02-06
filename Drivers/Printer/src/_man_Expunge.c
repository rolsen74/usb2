
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
struct PrinterBase *devBase;
PTR seglist;

	USBDEBUG( "Printer : _manager_Expunge" );

	devBase = (PTR) Self->Data.LibBase;

	if ( ! devBase->dev_Base.dd_Library.lib_OpenCnt )
	{
		seglist = devBase->dev_SegList;

		Resources_Free( devBase );

		Remove( (PTR) devBase );

		DeleteLibrary( (PTR) devBase );
		devBase = NULL;
	}
	else
	{
		devBase->dev_Base.dd_Library.lib_Flags |= LIBF_DELEXP;
		seglist = NULL;
	}

	return( seglist );
}

// --
