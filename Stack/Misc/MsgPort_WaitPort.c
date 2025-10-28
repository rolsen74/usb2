
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#if 0
void MsgPort_WaitPort( struct USB2_MsgPort *mp )
{
	if ( ! mp )
	{
		return;
	}

	if ( mp->ump_StructID != ID_VNCMPORT )
	{
		printf( "Invalid VNC MsgPort (%p)\n", (PTR) mp );
		return;
	}

	WaitPort( & mp->ump_MsgPort );
}
#endif

// --
