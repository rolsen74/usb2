
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Resources_Free( struct EthernetBase *ethBase UNUSED )
{
	if ( IUSB2 )
	{
		DropInterface( (PTR) IUSB2 );

		IUSB2 = NULL;
	}

	if ( USB2Base )
	{
		CloseDevice( USB2IOReq );

		USB2Base = NULL;
	}

	FreeSysObject( ASOT_IOREQUEST, USB2IOReq );

	FreeSysObject( ASOT_PORT, USB2MsgPort );

	USB2IOReq = NULL;
	USB2MsgPort = NULL;

	if ( IUtility )
	{
		DropInterface( (PTR) IUtility );
	}

	if ( UtilityBase )
	{
		CloseLibrary( UtilityBase );
	}

	if ( IDOS )
	{
		DropInterface( (PTR) IDOS );
	}

	if ( DOSBase )
	{
		CloseLibrary( DOSBase );
	}

	if ( INewlib )
	{
		DropInterface( INewlib );
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );
	}
}

// --
