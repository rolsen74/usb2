
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S8
Video_Stream_Close( struct VideoUnit *unit )
{
	S8 error;

	if ( unit->Stream )
	{
		Video_Read_Stop( unit );

		USB2_EPRes_Release( unit->Stream );

		unit->Stream = NULL;
	}

	if ( unit->USBDetached )
	{
		unit->OV534StopPending = FALSE;
		unit->StreamAltActive = FALSE;

		return ( USB2Err_Device_Detached );
	}

	if ( unit->OV534StopPending )
	{
		error = Video_OV534_Stop( unit );

		if ( error )
		{
			DebugPrintF( "USB Video: PlayStation Eye stop failed, USB error %ld\n", (S32) error );

			return ( unit->USBDetached ? USB2Err_Device_Detached : IOERR_OPENFAIL );
		}
	}

	if ( unit->StreamAltActive )
	{
		if ( ! Video_Stream_SetAlt( unit, 0 ) )
		{
			return ( IOERR_OPENFAIL );
		}

		unit->StreamAltActive = FALSE;
	}

	return ( 0 );
}

// --
