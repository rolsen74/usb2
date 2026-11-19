
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Video_Stream_Close( struct VideoUnit *unit )
{
	if ( unit->Stream )
	{
		Video_Read_Stop( unit );

		USB2_EPRes_Release( unit->Stream );

		unit->Stream = NULL;
	}

	if ( unit->StreamAltActive )
	{
		if ( ! Video_Stream_SetAlt( unit, 0 ) )
		{
			return ( FALSE );
		}

		unit->StreamAltActive = FALSE;
	}

	return ( TRUE );
}

// --
