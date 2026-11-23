
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
__Get_EndPoint_Size ( struct USB2_EPResource * epr, struct RealFunctionNode * fn, struct USB2_IORequest * ioreq,
					  struct USB2_SetupData * sd, struct USBBase * usbbase )
{
	S32 retval;
	U32 max;
	U32 cnt;
	U32 packet;

	retval = FALSE;

	// Only get 8 bytes of the Device Descriptor
	// so we can get the EndPoint Size

	cnt = 0;
	max = 3;

	while ( TRUE )
	{
		// Small delay (Retries and prev command)
		//		HCD_DELAY_MS( 25 );

		// Check max retries
		if ( max < cnt++ )
		{
			USBERROR ( "__Get_Device_Desc : Error Max trys : io_Error %ld", ioreq->io_Error );
			goto bailout;
		}

		sd->RequestType = REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device;
		sd->RequestCode = REQCODE_Get_Descriptor;
		sd->Value		= LE_SWAP16 ( DSCTYPE_Device << 8 );
		sd->Index		= LE_SWAP16 ( 0 );
		sd->Length		= LE_SWAP16 ( 8 );

		ioreq->io_Data		  = fn->fkt_DeviceDescriptor;
		ioreq->io_Length	  = 8;
		ioreq->io_Command	  = CMD_READ;
		ioreq->io_SetupData	  = sd;
		ioreq->io_SetupLength = sizeof ( struct USB2_SetupData );

		IO_DO ( ioreq );

		if ( InitCancelled( usbbase, fn, ioreq->io_Error ) )
		{
			goto bailout;
		}


		if ( ( ioreq->io_Error == USB2Err_NoError ) && ( ioreq->io_Actual == 8 ) )
		{
			break;
		}
	}

	packet = fn->fkt_DeviceDescriptor->MaxPacketSize0;

	if ( ( fn->fkt_DeviceDescriptor->DescriptorType != DSCTYPE_Device ) || ( ! packet ) )
	{
		goto bailout;
	}

	// The virtual root hub publishes a USB 1.x or 2.0 descriptor on every controller.
	if ( fn->fkt_Tier )
	{
		if ( ( ( fn->fkt_Speed >= USBSPEED_Super ) && ( packet != 9 ) )
			 || ( ( fn->fkt_Speed == USBSPEED_High ) && ( packet != 64 ) )
			 || ( ( fn->fkt_Speed == USBSPEED_Low ) && ( packet != 8 ) )
			 || ( ( fn->fkt_Speed == USBSPEED_Full ) && ( packet != 8 ) && ( packet != 16 ) && ( packet != 32 )
				  && ( packet != 64 ) ) )
		{
			USBERROR ( "__Get_EndPoint_Size : Invalid endpoint zero packet size" );

			goto bailout;
		}

		if ( fn->fkt_Speed >= USBSPEED_Super )
		{
			packet = 512;
		}
	}

	( (struct USB2_EndPointNode *)epr->EndPoint )->ep_MaxPacketSize = packet;
	epr->EndPointMax												= packet;

	retval = TRUE;

bailout:

	return ( retval );
}

// --
