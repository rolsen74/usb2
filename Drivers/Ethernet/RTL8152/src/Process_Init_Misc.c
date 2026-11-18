
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
unit_Process_InitMisc( struct devUnit *unit )
{
int retval;


    retval = FALSE;

	IExec->NewList( & unit->unit_RxList );
	IExec->NewList( & unit->unit_TxList );
	IExec->NewList( & unit->unit_EventsList );
	IExec->NewList( & unit->unit_SampleList );
	IExec->NewList( & unit->unit_MultiCastList );

	IExec->NewList( & unit->unit_USBBulkInActiveList );
	IExec->NewList( & unit->unit_USBBulkInFreeList );
	IExec->NewList( & unit->unit_USBBulkOutActiveList );
	IExec->NewList( & unit->unit_USBBulkOutFreeList );

	// --

	unit->unit_BeginIOMsgPort = IExec->AllocSysObjectTags( ASOT_PORT,
		TAG_END
	);

	if ( unit->unit_BeginIOMsgPort == NULL )
	{
		SHOWMSG( "Error Creating BeginIO MsgPort" );
		goto bailout;
	}

	unit->unit_BeginIOMsgPortBit = 1UL << unit->unit_BeginIOMsgPort->mp_SigBit;

	// --

	unit->unit_AbortIOMsgPort = IExec->AllocSysObjectTags( ASOT_PORT,
		TAG_END
	);

	if ( unit->unit_AbortIOMsgPort == NULL )
	{
		SHOWMSG( "Error Creating AbortIO MsgPort" );
		goto bailout;
	}

	unit->unit_AbortIOMsgPortBit = 1UL << unit->unit_AbortIOMsgPort->mp_SigBit;

	// --

    retval = TRUE;

bailout:
    return(	    retval );
}

// --
