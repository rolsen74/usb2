
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
__myProcess_Main_Direct( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct IOStdReq *pario;

	U32 ionr;

	while( TRUE )
	{
		USBERROR( "Printer : Main_Direct : USB Active %ld : PAR Waiting %ld : BUF Waiting %ld : BUF Free %ld :",
			unit->unit_USB_IOReq_ActiveCount,
			unit->unit_PAR_IOReq_WaitingCount,
			unit->unit_Buffer_WaitingCount,
			unit->unit_Buffer_FreeCount
		);

		usbio = (PTR) GetMsg( unit->unit_Bulk_Tx_Resource->MsgPort );

		USBERROR( "__myProcess_Main_Direct : ioreq %p", usbio );

		if ( ! usbio )
		{
			break;
		}

		ionr = usbio->io_UserValue;
		pario = usbio->io_UserData;

		USBERROR( "__myProcess_Main_Direct : pario %p (#%lu)", pario, ionr );

		// --

		#ifdef DO_DEBUG

		if ( ! pario )
		{
			USBERROR( "pario NULL Pointer" );
		}

		if ( ! unit->unit_Bulk_Tx_Active[ionr] )
		{
			USBERROR( "unit_Bulk_Tx_Active[%ld] is not Active", ionr );
		}

		if ( unit->unit_Bulk_Tx_Active[ionr] != usbio )
		{
			USBERROR( "unit_Bulk_Tx_Active[%ld] is wrong pointer", ionr );
		}

		#endif

		unit->unit_Bulk_Tx_Active[ionr] = NULL;
		unit->unit_USB_IOReq_ActiveCount--;

		// --

		switch( usbio->io_Error )
		{
			case USB2Err_NoError:
			{
				USBERROR( "__myProcess_Main_Direct : USB2Err_NoError" );

				pario->io_Actual = usbio->io_Actual;
				pario->io_Error = ( usbio->io_Actual == usbio->io_Length ) ? 0 : ParErr_LineErr;

				if ( ( usbio->io_Data ) && ( usbio->io_Actual > 0 ) && ( usbio->io_Actual <= usbio->io_Length ) )
				{
					U8 *buf;
					U32 off;

					buf = usbio->io_Data;
					off = usbio->io_Actual-1;

					unit->unit_LastByteSend = buf[ off ];
					unit->unit_JobHasData = TRUE;
				}

				break;
			}

			case USB2Err_Device_Detached:
			{
				// Todo : Flush all queued requests
				USBERROR( "__myProcess_Main_Direct : USB2Err_Device_Detached" );

				pario->io_Actual = 0;
				pario->io_Error = ParErr_NotOpen;
				unit->unit_Detached = TRUE;
				break;
			}

			case USB2Err_Host_Stall:
			{
				USBERROR( "__myProcess_Main_Direct : Destalling" );

				pario->io_Actual = usbio->io_Actual;
				pario->io_Error = ParErr_LineErr;

				if ( ! USB2_EPRes_Destall( unit->unit_Bulk_Tx_Resource ))
				{
					unit->unit_Detached = TRUE;
					pario->io_Error = ParErr_NotOpen;
				}

				break;
			}

			default:
			{
				USBERROR( "__myProcess_Main_Direct : Unknown error %lu", (U32) usbio->io_Error );

				pario->io_Actual = 0;
				pario->io_Error = ParErr_LineErr;
				break;
			}
		}

		// --

//		USBERROR( "usbio->io_Offset  : %ld", usbio->io_Offset );
//		USBERROR( "usbio->io_Length  : %ld", usbio->io_Length );
//		USBERROR( "usbio->io_Actual  : %ld", usbio->io_Actual );
//		USBERROR( "usbio->io_Error   : %ld", usbio->io_Error );

//		USBERROR( "pario->io_Offset  : %ld", pario->io_Offset );
//		USBERROR( "pario->io_Length  : %ld", pario->io_Length );
//		USBERROR( "pario->io_Actual  : %ld", pario->io_Actual );
//		USBERROR( "pario->io_Error   : %ld", pario->io_Error );

		USBERROR( "__myProcess_Main_Direct : Reply ParIO %p", pario );

		if ( pario->io_Flags & IOPARF_ABORT )
		{
			pario->io_Actual = usbio->io_Actual;
			pario->io_Error = IOERR_ABORTED;
		}

		ReplyMsg( (PTR) pario );

		AddTail( unit->unit_USB_IOReq_FreeList, (PTR) usbio );
	}

	Direct_Schedule( unit );
}
