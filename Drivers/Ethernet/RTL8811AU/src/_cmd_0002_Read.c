
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"

// --

S32
Cmd_Read( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq )
{
	if ( ! RTL8811AU_DataAllowed( unit, ioreq ) )
	{
		ReportUnitOffline( unit, ioreq );

		return ( FALSE );
	}

	if ( ( ! ioreq->ios2_Data ) || ( ( ! opener->bm_CopyHook ) && ( ! opener->bm_CopyToBuff ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_BUFF_ERROR;

		return ( FALSE );
	}

	ioreq->ios2_Req.io_Flags &= ~IOF_QUICK;

	AddTail( & unit->unit_Pending, (PTR) ioreq );

	return ( TRUE );
}

// --
