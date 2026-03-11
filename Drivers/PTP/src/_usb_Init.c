
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

U32 _usb_Init( struct USB_Struct *us )
{
struct USB2_IORequest *ioreq;
struct PTP_Container *condata;
U32 retval;
U8 data[512];

	MYINFO( "PTP-USB : _usb_Init" );

	retval = FALSE;
	
	// --

	us->us_CmdMsgPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! us->us_CmdMsgPort )
	{
		MYERROR( "PTP-USB : Error creating MsgPort" );
		goto bailout;
	}

	// --

	us->us_Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, us->us_StartMsg,
		TAG_END
	);

	if ( ! us->us_Register )
	{
		MYERROR( "PTP-USB : Error Registering Device" );
		goto bailout;
	}

	us->us_Res_Control = us->us_Register->Res_Control;

	// --

	us->us_Res_Interrupt = USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ! us->us_Res_Interrupt )
	{
		MYERROR( "PTP-USB : Error obtaining IntIn EndPoint Resource" );
		goto bailout;
	}

	// --

	us->us_Res_BulkIn = USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		TAG_END
	);

	if ( ! us->us_Res_BulkIn )
	{
		MYERROR( "PTP-USB : Error obtaining BulkIn EndPoint Resource" );
		goto bailout;
	}

	// --

	us->us_Res_BulkOut = USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_AddZeroPacket, TRUE,
		TAG_END
	);

	if ( ! us->us_Res_BulkOut )
	{
		MYERROR( "PTP-USB : Error obtaining BulkOut EndPoint Resource" );
		goto bailout;
	}

	// --
	// Reset Start IDs
	us->us_SessionID = 1;
	us->us_TransactionID = 0;

	// Only add to TransID at start cmd, like here
	us->us_TransactionID++;

	// Open Session
	condata = (PTR) data;
	condata->Length		= LE_SWAP32( 12 + 1*4 );
	condata->Type		= LE_SWAP16( CTYPE_Command );
	condata->Code		= LE_SWAP16( PTP_Open_Session );
	condata->TransID	= LE_SWAP32( us->us_TransactionID );
	condata->Parms[0]	= LE_SWAP32( us->us_SessionID );

	// Send
	ioreq = us->us_Res_BulkOut->IORequests[0];
	ioreq->io_Data = condata;
	ioreq->io_Length = 12 + 1*4;
	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error )
	{
		MYERROR( "PTP-USB : Error sending %ld", (S32) ioreq->io_Error );
		goto bailout;
	}

	// Read -- use default buffer
	ioreq = us->us_Res_BulkIn->IORequests[0];
	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error )
	{
		MYERROR( "PTP-USB : Error reading %ld", (S32) ioreq->io_Error );
		goto bailout;
	}

	if ( ioreq->io_Length < 12 )
	{
		MYERROR( "PTP-USB : Invalid Reposnce Length %ld", (S32) ioreq->io_Length );
		goto bailout;
	}

	condata = (PTR) ioreq->io_Data;

	if ( LE_SWAP32( condata->Length ) != 12 )
	{
		MYERROR( "PTP-USB : Invalid Reposnce 2 Length %ld", (S32) LE_SWAP32( condata->Length ) );
		goto bailout;
	}

	if ( LE_SWAP16( condata->Type ) != CTYPE_Response )
	{
		MYERROR( "PTP-USB : Invalid Container Type %ld", (S32) LE_SWAP16( condata->Type ) );
		goto bailout;
	}

	if ( LE_SWAP32( condata->TransID ) != us->us_TransactionID )
	{
		MYERROR( "PTP-USB : TransactionID mismatch $%08lx", (S32) LE_SWAP32( condata->TransID ) );
		goto bailout;
	}

	if ( LE_SWAP16( condata->Code ) != 0x2001 )
	{
		MYERROR( "PTP-USB : Returned error $%04lx", (S32) LE_SWAP16( condata->Code ) );
		goto bailout;
	}

	// --



	
	// Only add to TransID at start cmd, like here
	us->us_TransactionID++;

	// Open Session
	condata = (PTR) data;
	condata->Length		= LE_SWAP32( 12 + 0*4 );
	condata->Type		= LE_SWAP16( CTYPE_Command );
	condata->Code		= LE_SWAP16( PTP_Get_Device_Info );
	condata->TransID	= LE_SWAP32( us->us_TransactionID );

	// Send
	ioreq = us->us_Res_BulkOut->IORequests[0];
	ioreq->io_Data = condata;
	ioreq->io_Length = 12 + 0*4;
	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error )
	{
		MYERROR( "PTP-USB : Error sending %ld", (S32) ioreq->io_Error );
		goto bailout;
	}


	
	// Read data
	ioreq = us->us_Res_BulkIn->IORequests[0];
	ioreq->io_Data = data;
	ioreq->io_Length = 512;		// fix me: hard coded maxpacket size (its ehci)

	S32 full;
	S32 once = TRUE;
	S32 llen = 0;
	PTR buf = NULL;
	PTR cur;

	do
	{
		DoIO( (PTR) ioreq );

		if ( ioreq->io_Error )
		{
			MYERROR( "PTP-USB : Error reading %ld", (S32) ioreq->io_Error );
			goto bailout;
		}

		if ( once )
		{
			once = FALSE;

			// Need header
			if ( ioreq->io_Length < 12 )
			{
				MYERROR( "PTP-USB : Invalid Reposnce Length %ld", (S32) ioreq->io_Length );
				goto bailout;
			}

			// Removed header, so we have real data size, recived
			ioreq->io_Length -= 12;

			condata = (PTR) ioreq->io_Data;

			if ( LE_SWAP16( condata->Type ) != CTYPE_Data )
			{
				MYERROR( "PTP-USB : Invalid Container Type %ld", (S32) LE_SWAP16( condata->Type ) );
				goto bailout;
			}

			if ( LE_SWAP32( condata->TransID ) != us->us_TransactionID )
			{
				MYERROR( "PTP-USB : TransactionID mismatch $%08lx", (S32) LE_SWAP32( condata->TransID ) );
				goto bailout;
			}

			// get full data buffer size
			llen = LE_SWAP32( condata->Length ) - 12;
			full = llen;

			cur = buf = AllocVecTags( llen,
				TAG_END
			);

			if ( ! buf )
			{
				MYERROR( "PTP-USB : Out of mem %ld", (S32) llen );
				goto bailout;
			}

			memcpy( buf, & data[12], MIN( llen, (S32) ioreq->io_Length ));
		}
		else
		{
			memcpy( cur, data, MIN( llen, (S32) ioreq->io_Length ));
		}

		llen	-= ioreq->io_Length;
		cur		+= ioreq->io_Length;
		MYERROR( "PTP-USB : Data %ld bytes now %ld/%ld", (S32) ioreq->io_Error, llen, full );
	}
	while( llen > 0 );

	// --
	// buf = should hold data now

	// Read responce
	ioreq = us->us_Res_BulkIn->IORequests[0];
	DoIO( (PTR) ioreq );

	if ( ioreq->io_Error )
	{
		MYERROR( "PTP-USB : Error reading %ld", (S32) ioreq->io_Error );
		goto bailout;
	}

	if ( ioreq->io_Length < 12 )
	{
		MYERROR( "PTP-USB : Invalid Reposnce Length %ld", (S32) ioreq->io_Length );
		goto bailout;
	}

	condata = (PTR) ioreq->io_Data;

	if ( LE_SWAP32( condata->Length ) != 12 )
	{
		MYERROR( "PTP-USB : Invalid Reposnce 2 Length %ld", (S32) LE_SWAP32( condata->Length ) );
		goto bailout;
	}

	if ( LE_SWAP16( condata->Type ) != CTYPE_Response )
	{
		MYERROR( "PTP-USB : Invalid Container Type %ld", (S32) LE_SWAP16( condata->Type ) );
		goto bailout;
	}

	if ( LE_SWAP32( condata->TransID ) != us->us_TransactionID )
	{
		MYERROR( "PTP-USB : TransactionID mismatch $%08lx", (S32) LE_SWAP32( condata->TransID ) );
		goto bailout;
	}

	if ( LE_SWAP16( condata->Code ) != 0x2001 )
	{
		MYERROR( "PTP-USB : Returned error $%04lx", (S32) LE_SWAP16( condata->Code ) );
		goto bailout;
	}

	// --

	MYERROR( "Yay Session Open" );

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
