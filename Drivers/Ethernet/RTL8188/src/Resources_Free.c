
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void Resources_Free( struct EthernetBase *ethBase UNUSED )
{
	USBDEBUG( "RTL8188 : Resources_Free" );

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

	if ( USB2IOReq )
	{
		FreeSysObject( ASOT_IOREQUEST, USB2IOReq );

		USB2IOReq = NULL;
	}

	if ( USB2MsgPort )
	{
		FreeSysObject( ASOT_PORT, USB2MsgPort );

		USB2MsgPort = NULL;
	}

	// --

	if ( IRTL8188Timer )
	{
		DropInterface( (PTR) IRTL8188Timer );

		IRTL8188Timer = NULL;
	}

	if ( IUtility )
	{	
		DropInterface( (PTR) IUtility );

		IUtility = NULL;
	}

	if ( UtilityBase )
	{
		CloseLibrary( (PTR) UtilityBase );
	}

	// --

	if ( IDOS )
	{	
		DropInterface( (PTR) IDOS );
	}

	if ( DOSBase )
	{
		CloseLibrary( (PTR) DOSBase );
	}

	// --

	if ( INewlib )
	{	
		DropInterface( INewlib );
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );
	}

	// --
}

// --
