
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
Finish_Job( struct PrinterUnit *unit )
{
	if ( ! unit->unit_PortInterface->JobClosing( unit->unit_PortUnit ) )
	{
		return;
	}

	if ( unit->unit_Buffer_Size )
	{
		Buffer_Flush( unit );
	}

	if ( ( unit->unit_USB_IOReq_ActiveCount ) || ( unit->unit_PAR_IOReq_WaitingCount ) ||
		( unit->unit_Buffer_WaitingCount ) )
	{
		return;
	}

	if ( ( ! unit->unit_Dot4_Active ) && ( unit->unit_JobHasData ) && ( unit->unit_LastByteSend != '\f' ) )
	{
		Send_FormFeed( unit );
	}

	unit->unit_JobHasData = FALSE;

	unit->unit_PortInterface->FinishJob( unit->unit_PortUnit );
}

// --
