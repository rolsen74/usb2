
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE U32
Audio_ClockRate( struct AudioUnit *unit )
{
	struct USB2_EPResource *control;
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *setup;

	U8 rate[4];

	control = unit->Register->Res_Control;
	ioreq = control->IORequests[0];
	setup = control->SetupData;
	setup->RequestType = REQTYPE_Read | REQTYPE_Class | REQTYPE_Interface;
	setup->RequestCode = 0x01; // UAC2 CUR
	setup->Value = LE_SWAP16( 0x0100 ); // Clock sampling frequency
	setup->Index = LE_SWAP16( ( (U16) unit->ClockEntity << 8 ) | unit->ClockInterface );
	setup->Length = LE_SWAP16( sizeof( rate ) );
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = rate;
	ioreq->io_Length = sizeof( rate );
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );

	DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;

	if ( ( ioreq->io_Error ) || ( ioreq->io_Actual != sizeof( rate ) ) )
	{
		AUDIO_TRACE( "clock %lu interface %lu GET CUR failed, USB error %ld\n",
			(U32) unit->ClockEntity, (U32) unit->ClockInterface, (S32) ioreq->io_Error );

		return ( 0 );
	}

	return ( (U32) rate[0] | ( (U32) rate[1] << 8 ) | ( (U32) rate[2] << 16 ) | ( (U32) rate[3] << 24 ) );
}

// --
