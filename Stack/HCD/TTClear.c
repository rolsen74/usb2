
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

struct HCD_TTClear
{
	struct USB2_SetupData Setup;
	struct RealRequest *  Original;
	S32					  BothDirections;
};

#define HCD_CLEAR_TT_BUFFER 8U
#define HCD_TT_DIRECTION_IN 0x8000U

// --

SEC_CODE S32
HCD_Clear_TT ( struct USB2_HCDNode * hn, struct RealRequest * ioreq, struct RealFunctionNode * hub, U32 address, U32 index )
{
	struct USBBase *		   usbbase;
	struct USB2_EndPointNode * ep;
	struct RealRequest *	   clear;
	struct HCD_TTClear *	   work;

	U32 value;

	usbbase = hn->hn_USBBase;
	ep		= ioreq->req_EndPoint;

	if ( ( ! hub->fkt_Tier ) || ( hub->fkt_Detach ) || ( hub->fkt_HCDGeneration != hn->hn_Generation ) )
	{
		return ( FALSE );
	}

	work = MEM_ALLOCVEC ( sizeof ( *work ), TRUE );

	if ( ! work )
	{
		return ( FALSE );
	}

	clear = IOREQUEST_ALLOCTAGS ( USB2Tag_IOReq_EndPoint, hub->fkt_ControlEndPoint, USB2Tag_IOReq_TimeOut, 500000,
								  USB2Tag_IOReq_MsgPort, &hn->hn_TTClearPort.ump_MsgPort, TAG_END );

	if ( ! clear )
	{
		MEM_FREEVEC ( work );

		return ( FALSE );
	}

	work->Original		 = ioreq;
	work->BothDirections = ep->ep_Type == EPATT_Type_Control;
	value				 = ep->ep_Number | ( address << 4 ) | ( ep->ep_Type << 11 );

	if ( ( ioreq->req_Public.io_Command == CMD_READ ) != work->BothDirections )
	{
		value |= HCD_TT_DIRECTION_IN;
	}

	work->Setup.RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Other;
	work->Setup.RequestCode = HCD_CLEAR_TT_BUFFER;
	work->Setup.Value		= LE_SWAP16 ( value );
	work->Setup.Index		= LE_SWAP16 ( index );

	clear->req_Public.io_Command	 = CMD_WRITE;
	clear->req_Public.io_SetupData	 = &work->Setup;
	clear->req_Public.io_SetupLength = sizeof ( work->Setup );
	clear->req_Public.io_UserData	 = work;

	// Keep later requests queued while the HCD services the hub's control request.
	ep->ep_Request_Active = ioreq;
	hn->hn_TTClearPending++;

	IO_SEND ( (PTR)&clear->req_Public );

	return ( TRUE );
}

// --

SEC_CODE void
HCD_TT_Complete ( struct USB2_HCDNode * hn )
{
	struct USBBase *		   usbbase;
	struct USB2_EndPointNode * ep;
	struct RealRequest *	   clear;
	struct RealRequest *	   ioreq;
	struct HCD_TTClear *	   work;

	S32 error;
	S32 current;

	usbbase = hn->hn_USBBase;

	while ( ( clear = MSGPORT_GETMSG ( &hn->hn_TTClearPort ) ) )
	{
		work	= clear->req_Public.io_UserData;
		ioreq	= work->Original;
		error	= clear->req_Public.io_Error;
		current = ( hn->hn_AcceptIO ) && ( ! ioreq->req_Function->fkt_Detach )
				  && ( ioreq->req_Function->fkt_HCDGeneration == hn->hn_Generation );

		if ( ( ! error ) && ( work->BothDirections ) && ( current ) )
		{
			work->BothDirections = FALSE;
			work->Setup.Value ^= LE_SWAP16 ( HCD_TT_DIRECTION_IN );

			IO_SEND ( (PTR)&clear->req_Public );

			continue;
		}

		IOREQUEST_FREE ( clear );

		MEM_FREEVEC ( work );

		hn->hn_TTClearPending--;
		ep = ioreq->req_EndPoint;

		ENDPOINT_LOCK ( ep );

		ep->ep_Request_Active = NULL;

		if ( ioreq->req_AbortRequested )
		{
			ioreq->req_Public.io_Error = IOERR_ABORTED;
		}
		else if ( ( ! current ) || ( error == USB2Err_Device_Detached ) )
		{
			ioreq->req_Public.io_Error = USB2Err_Device_Detached;
		}

		if ( ( error ) && ( error != USB2Err_Device_Detached ) && ( current ) )
		{
			usbbase->usb_IExec->DebugPrintF ( "USB : HCD %lu hub translator clear failed (%ld)\n", hn->hn_HCDIndex, error );

			USB2_HCD_RequestReset ( hn );
		}

		HCD_REPLY ( ioreq );

		if ( ( current ) && ( error != USB2Err_Device_Detached ) )
		{
			HCD_RESTART_ENDPOINT ( hn, ep );
		}

		ENDPOINT_UNLOCK ( ep );
	}
}

// --
