
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

/*
** This function removes the IORequest from active chip lists.
**
** Note: Must be running in the HCD stack context, make sure AbortIO obays this
*/

SEC_CODE void
__HCD_Transfer_Remove ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct RealRequest * ioreq )
{
	struct USB2_EndPointNode * destall_ep;
	struct USB2_EndPointNode * ep;
	struct RealFunctionNode *  fn;

	S32 active;
	U32 actual;
	U32 index;

	//	USBDEBUG( "__HCD_Transfer_Remove :" );
	TASK_NAME_ENTER ( "HCD : __HCD_Transfer_Remove" );

	// --

	fn = ioreq->req_Function;
	ep = ioreq->req_EndPoint;

#ifdef DO_PANIC

	if ( FUNCTION_VALID ( fn ) != VSTAT_Okay )
	{
		USBPANIC ( "__HCD_Transfer_Remove : Invalid fn %p", fn );
	}

	if ( ENDPOINT_VALID ( ep ) != VSTAT_Okay )
	{
		USBPANIC ( "__HCD_Transfer_Remove : Invalid ep %p", ep );
	}

#endif

	// --

	/*
	** Abort Timer IO Request
	*/

	if ( ioreq->req_TimerAdded )
	{
		IO_STOP ( &ioreq->req_TimerIOReq );
		ioreq->req_TimerAdded = FALSE;
	}

	// --
	// Remove Active IOReq

	active = ioreq->req_PublicStat == IORS_HCD_Active;

	if ( active )
	{
		// EHCI's DoorBell need the HCD Context

		switch ( ep->ep_Type )
		{
			case EPATT_Type_Control:
			{
				if ( hn->HCD_Functions.Control_Remove )
				{
					hn->HCD_Functions.Control_Remove ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Control_Remove not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Interrupt:
			{
				if ( hn->HCD_Functions.Interrupt_Remove )
				{
					hn->HCD_Functions.Interrupt_Remove ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Interrupt_Remove not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Bulk:
			{
				if ( hn->HCD_Functions.Bulk_Remove )
				{
					USBDEBUG ( "__HCD_Transfer_Remove : Bulk_Remove" );
					hn->HCD_Functions.Bulk_Remove ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Bulk_Remove not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Isochronous:
			{
				if ( hn->HCD_Functions.Isochronous_Remove )
				{
					USBDEBUG ( "__HCD_Transfer_Remove : Isochronous_Remove" );
					hn->HCD_Functions.Isochronous_Remove ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Isochronous_Remove not Implemented in HCD" );
				}
				break;
			}

			default:
			{
				USBDEBUG ( "__HCD_Transfer_Remove : Invalid ep_Type %ld", (S32)ep->ep_Type );
				break;
			}
		}

		if ( ! hn->hn_AcceptIO )
		{
			TASK_NAME_LEAVE ();

			return;
		}

		// hmm Functions may not be implemented..
		// so clear those or should I ??
		// They are after all still active
		ioreq->req_PublicStat = IORS_HCD;

		if ( ep->ep_Request_Active == ioreq )
		{
			ep->ep_Request_Active = ep->ep_Request_ActiveNext;
		}

		ep->ep_Request_ActiveNext = NULL;
	}

	// --

	/*
	** Count transfered bytes and if a Read command copy data to user buffer
	*/

	actual = 0;

	if ( ( ioreq->req_Public.io_Error == USB2Err_NoError ) ||
		( ( hn->hn_HCDType == HCDTYPE_UHCI ) && ( active ) ) )
	{
		switch ( ep->ep_Type )
		{
			case EPATT_Type_Control:
			{
				if ( hn->HCD_Functions.Control_Length )
				{
					actual = hn->HCD_Functions.Control_Length ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Control_Length not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Interrupt:
			{
				if ( hn->HCD_Functions.Interrupt_Length )
				{
					actual = hn->HCD_Functions.Interrupt_Length ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Interrupt_Length not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Bulk:
			{
				if ( hn->HCD_Functions.Bulk_Length )
				{
					USBDEBUG ( "__HCD_Transfer_Remove : Bulk_Length" );
					actual = hn->HCD_Functions.Bulk_Length ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Bulk_Length not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Isochronous:
			{
				if ( hn->HCD_Functions.Isochronous_Length )
				{
					USBDEBUG ( "__HCD_Transfer_Remove : Isochronous_Length" );
					actual = hn->HCD_Functions.Isochronous_Length ( hn, ioreq );
				}
				else
				{
					USBERROR ( "__HCD_Transfer_Remove : Isochronous_Length not Implemented in HCD" );
				}
				break;
			}

			default:
			{
				USBERROR ( "__HCD_Transfer_Remove : Invalid ep_Type %ld", (S32)ep->ep_Type );
				break;
			}
		}
	}

	ioreq->req_Public.io_Actual = actual;

	// --
	// Check if a Destall succeded

	if ( ioreq->req_DestallTarget )
	{
		destall_ep = ioreq->req_DestallTarget;
		index	   = LE_SWAP16 ( ioreq->req_Public.io_SetupData->Index );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		if ( ( active ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe )
			 && ( destall_ep->ep_Active ) && ( ioreq->req_Public.io_Error == USB2Err_NoError ) )
		{
			usbbase->usb_IExec->DebugPrintF ( "[%s] We successfully Destalled -- Addr %ld : EP    $%02lx : EPN Info : EPNr "
											  "%ld : EPDir %ld : EPSize %ld : EPType %ld\n",
											  usbbase->usb_IExec->FindTask ( NULL )->tc_Node.ln_Name, fn->fkt_Address, index,
											  destall_ep->ep_Number, destall_ep->ep_Direction, destall_ep->ep_MaxPacketSize,
											  destall_ep->ep_Type );

			destall_ep->ep_DataToggle = 0;
			destall_ep->ep_Stalled	  = FALSE;
		}
		else
		{
			usbbase->usb_IExec->DebugPrintF ( "[%s] Destalled Failed -- Addr %ld -- EP $%02lx\n",
											  usbbase->usb_IExec->FindTask ( NULL )->tc_Node.ln_Name, fn->fkt_Address, index );
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
	}

	// --
	// Set stall flags, if needed

	if ( ( ioreq->req_Public.io_Error == USB2Err_Host_Stall ) && ( ep->ep_Type != EPATT_Type_Control ) )
	{
		ep->ep_Stalled = TRUE;
	}

	// --

	/*
	** We are done now, release resources
	*/

	if ( hn->HCD_Functions.Transfer_Free )
	{
		hn->HCD_Functions.Transfer_Free ( hn, ioreq );
	}
	else
	{
		USBERROR ( "__HCD_Transfer_Remove : Transfer_Free not Implemented in HCD" );
	}

	// --

	TASK_NAME_LEAVE ();
}

// --
