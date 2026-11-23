
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
Direct_Schedule( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct IOStdReq *pario;

	U32 ionr;

	if ( ( unit->unit_Detached ) || ( unit->unit_JobError ) )
	{
		return;
	}

	if ( unit->unit_Dot4_Active )
	{
		while(( pario = (PTR) RemHead( unit->unit_PAR_IOReq_WaitingList )))
		{
			U32 length;
			U32 written;
			S32 success;

			if ( pario->io_Flags & IOPARF_ABORT )
			{
				unit->unit_PAR_IOReq_WaitingCount--;
				pario->io_Error = IOERR_ABORTED;

				ReplyMsg( (PTR) pario );

				continue;
			}

			length = ( pario->io_Length == -1U )
				? strlen( pario->io_Data ) : pario->io_Length;

			written = 0;

			unit->unit_PAR_IOReq_WaitingCount--;

			success = Dot4_Write( unit, pario->io_Data, length, &written );

			pario->io_Actual = written;
			pario->io_Error = success ? 0 : ( unit->unit_Detached ? ParErr_NotOpen : ParErr_LineErr );

			if ( written )
			{
				unit->unit_LastByteSend = ((U8 *) pario->io_Data)[written-1];
			}

			if ( pario->io_Flags & IOPARF_ABORT )
			{
				pario->io_Error = IOERR_ABORTED;
			}

			ReplyMsg( (PTR) pario );

			if ( ! success )
			{
				unit->unit_Detached = TRUE;
				break;
			}
		}

		return;
	}

	while( TRUE )
	{
		USBERROR( "Printer : Direct_Schedule : USB Active %ld : PAR Waiting %ld : BUF Waiting %ld : BUF Free %ld :",
			unit->unit_USB_IOReq_ActiveCount,
			unit->unit_PAR_IOReq_WaitingCount,
			unit->unit_Buffer_WaitingCount,
			unit->unit_Buffer_FreeCount
		);

		usbio = (PTR) GetHead( unit->unit_USB_IOReq_FreeList );

		USBERROR( "Printer : usbio %p", usbio );

		if ( ! usbio )
		{
			// No free USB IORequests
			USBINFO( "Direct_Schedule : No free USB IORequests" );

			break;
		}

		pario = (PTR) GetHead( unit->unit_PAR_IOReq_WaitingList );

		USBERROR( "Printer : pario %p", pario );

		if ( ! pario )
		{
			// Okay nothing too print
			USBINFO( "Direct_Schedule : Okay nothing too print" );

			break;
		}

		// Get IOReq nr
		ionr = usbio->io_UserValue;

		#ifdef DO_DEBUG

		if ( unit->unit_Bulk_Tx_Active[ionr] )
		{
			USBERROR( "unit_Bulk_Tx_Active[%ld] is not Free", ionr );

			break;
		}

		#endif

		// Move from Waiting/Free list into Active
		Remove( (PTR) pario );

		unit->unit_PAR_IOReq_WaitingCount--;

		if ( pario->io_Flags & IOPARF_ABORT )
		{
			pario->io_Error = IOERR_ABORTED;

			ReplyMsg( (PTR) pario );

			continue;
		}

		Remove( (PTR) usbio );

		unit->unit_Bulk_Tx_Active[ionr] = usbio;
		unit->unit_USB_IOReq_ActiveCount++;

		USBERROR( "Sending : PAR : Data %p : Len %lu", pario->io_Data, pario->io_Length );

		// Init and Start USB IORequest
		usbio->io_Command	= CMD_WRITE;
		usbio->io_UserData	= pario;
//		usbio->io_Offset	= 0;
		usbio->io_Length	= ( pario->io_Length == -1U ) ? strlen( pario->io_Data ) : pario->io_Length;

		usbio->io_Data		= pario->io_Data;

		SendIO( (PTR) usbio );
	}


}

// --
