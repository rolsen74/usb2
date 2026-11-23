
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32 __Get_Device_Stat( 
	struct RealFunctionNode *fn,
	struct USB2_IORequest *ioreq,
	struct USB2_SetupData *sd,
	struct USBBase *usbbase )
{
struct USB2_DeviceStatus stat;
S32 retval;
U32 max;
U32 cnt;
S32 short_reply;

	retval = FALSE;
	short_reply = FALSE;

	// Now we know the Real EndPoint Size
	// so get Full Device Descriptor

	cnt = 0;
	max = 3;

	while( TRUE )
	{
		// Small delay (Retries and prev command)
//		HCD_DELAY_MS( 25 );

		// Check max retries
		if ( max < cnt++ )
		{
			if ( short_reply )
			{
				fn->fkt_SelfPowered = FALSE;
				retval = TRUE;
			}
			else
			{
				USBERROR( "__Get_Device_Stat : Error Max trys : io_Error %ld", ioreq->io_Error );
			}

			goto bailout;
		}

		sd->RequestType	= REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device;
		sd->RequestCode	= REQCODE_Get_Status;
		sd->Value		= LE_SWAP16( 0 );
		sd->Index		= LE_SWAP16( 0 );
		sd->Length		= LE_SWAP16( sizeof( stat ));

		ioreq->io_Data	= & stat ;
		ioreq->io_Length= sizeof( stat );

		IO_DO(ioreq);

		if ( InitCancelled( usbbase, fn, ioreq->io_Error ) )
		{
			goto bailout;
		}

		if ( ioreq->io_Error == USB2Err_NoError )
		{
			if ( ioreq->io_Actual == sizeof( stat ) )
			{
				break;
			}

			short_reply = TRUE;
		}
	}

	fn->fkt_SelfPowered = ( LE_SWAP16( stat.Stat ) & USBSTATUS_Device_SelfPowered ) != 0;
	retval = TRUE;

bailout:

	return( retval );
}

// --
