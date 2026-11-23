
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
\\
// Returns:
\\   TRUE  - if Transfer was handled and is finish.
//   FALSE - if Transfer need more time to finish
\\
*/

SEC_CODE static inline S32
myAdd_Request ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct RealRequest * ioreq )
{
	void ( *add ) ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
	struct USB2_EndPointNode * ep;
	struct RealFunctionNode *  fn;
	S32						   handled;
	S32						   do_req;
	S32						   err;
	U32						   timeout;
	U32						   current;
	U32						   remaining;

	handled = FALSE;

	add = NULL;
	err = USB2Err_NoError;
	fn	= ioreq->req_Function;
	ep	= ioreq->req_EndPoint;

	if ( ( hn->hn_IsEHCI ) && ( ep->ep_Type == EPATT_Type_Interrupt ) && ( ep->ep_Transactions > 1 ) )
	{
		ioreq->req_Public.io_Error = USB2Err_Host_NoBandWidth;

		TASK_NAME_LEAVE ();

		return ( TRUE );
	}

	// --
	// Endpoint Stall Info:
	// - Control (Setup Stage)	--> Never stalls
	// - Control (Data/Status)	--> Can stall
	// - Bulk					--> Can stall
	// - Interrupt				--> Can stall
	// - Isochronous			--> Never stalls
	//
	// --
	// First figure out if its a Clear Halt (Destall)

	if ( ioreq->req_DestallTarget )
	{
		U32 index;

		index = LE_SWAP16 ( ioreq->req_Public.io_SetupData->Index );

		usbbase->usb_IExec->Disable ();

		usbbase->usb_IExec->DebugPrintF ( "[%s] Destalling Pipe -- Addr %ld -- EP $%02lx\n",
										  usbbase->usb_IExec->FindTask ( NULL )->tc_Node.ln_Name, (U32)fn->fkt_Address, index );

		usbbase->usb_IExec->Enable ();
	}

	// --
	// Now figure out if we can execute the Request
	// or maybe the pipe is stalled

	if ( ( ep->ep_Type == EPATT_Type_Interrupt ) || ( ep->ep_Type == EPATT_Type_Bulk ) )
	{
		// Check if EP Pipe is stalled
		if ( ep->ep_Stalled )
		{
			//			usbbase->usb_IExec->Disable();
			//			usbbase->usb_IExec->DebugPrintF( "Pipe is stalled -- Addr %ld -- EP $%02lx\n", (U32) fn->fkt_Address,
			//(U32) ep->ep_Number ); 			usbbase->usb_IExec->Enable();

			handled = TRUE;
			do_req	= FALSE;
			err		= USB2Err_Host_Stall;
		}
		else
		{
			do_req = TRUE;
		}
	}
	else
	{
		// A control protocol STALL is cleared by the next SETUP.
		do_req = TRUE;
	}

	if ( do_req )
	{
		switch ( ep->ep_Type )
		{
			case EPATT_Type_Control: // 0x00
			{
				if ( hn->HCD_Functions.Control_Build )
				{
					handled = hn->HCD_Functions.Control_Build ( hn, ioreq );
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add : Control_Build : Not Implemented in HCD" );
					handled = TRUE;
					err		= USB2Err_Host_IllegalEPType;
				}

				if ( hn->HCD_Functions.Control_Add )
				{
					add = hn->HCD_Functions.Control_Add;
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add : Control_Add : Not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Isochronous: // 0x01
			{
				if ( hn->HCD_Functions.Isochronous_Build )
				{
					handled = hn->HCD_Functions.Isochronous_Build ( hn, ioreq );
				}
				else
				{
					handled = TRUE;
					err		= USB2Err_Host_IllegalEPType;
				}
				if ( hn->HCD_Functions.Isochronous_Add )
				{
					add = hn->HCD_Functions.Isochronous_Add;
				}
				break;
			}

			case EPATT_Type_Bulk: // 0x02
			{
				if ( hn->HCD_Functions.Bulk_Build )
				{
					handled = hn->HCD_Functions.Bulk_Build ( hn, ioreq );
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add Bulk_Build : Not Implemented in HCD" );
					handled = TRUE;
					err		= USB2Err_Host_IllegalEPType;
				}

				if ( hn->HCD_Functions.Bulk_Add )
				{
					add = hn->HCD_Functions.Bulk_Add;
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add : Bulk_Add : Not Implemented in HCD" );
				}
				break;
			}

			case EPATT_Type_Interrupt: // 0x03
			{
				if ( hn->HCD_Functions.Interrupt_Build )
				{
					handled = hn->HCD_Functions.Interrupt_Build ( hn, ioreq );
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add : Interrupt_Build : Not Implemented in HCD" );
					handled = TRUE;
					err		= USB2Err_Host_IllegalEPType;
				}

				if ( hn->HCD_Functions.Interrupt_Add )
				{
					add = hn->HCD_Functions.Interrupt_Add;
				}
				else
				{
					USBERROR ( "HCD_myRequest_Add : Interrupt_Add : Not Implemented in HCD" );
				}
				break;
			}

			default:
			{
				USBERROR ( "HCD_myRequest_Add : Unsupported transfer type %lu -- Implement me", ep->ep_Type );
				handled = TRUE;
				err		= USB2Err_Host_IllegalEPType;
				break;
			}
		}

		if ( ! handled )
		{
			// --
			// We are good, now mark EndPoint as Active
			if ( ep->ep_Request_Active )
			{
				ep->ep_Request_ActiveNext = ioreq;
			}
			else
			{
				ep->ep_Request_Active = ioreq;
			}

			// --
			// Time to activate it

			ioreq->req_PublicStat = IORS_HCD_Active;
			NODE_ADDTAIL ( &hn->hn_Active_Transfer_List, ioreq );

			if ( add )
			{
				add ( hn, ioreq );
			}
			else
			{
				USBERROR ( "HCD_myRequest_Add : xx_Add : Not Implemented in HCD" );
			}

			// --
			// Start Timer : if requested --

			if ( ioreq->req_Public.io_TimeOut )
			{
				timeout = ioreq->req_Public.io_TimeOut;

				if ( ( hn->hn_IsEHCI ) && ( ep->ep_Type == EPATT_Type_Isochronous )
					 && ( fn->fkt_Speed == USBSPEED_Full ) && ( ioreq->req_Public.io_Command == CMD_WRITE )
					 && ( ioreq->req_Public.io_IsoPacketLengths ) )
				{
					current	  = PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_FRINDEX );
					remaining = ( ioreq->req_HCD.EHCI.IsoLastFrame - current ) & EHCI_FRINDEX_MASK;

					if ( remaining > EHCI_FRINDEX_MASK / 2 )
					{
						remaining = 0;
					}

					// A missed final split may never raise its completion interrupt.
					timeout = MIN ( timeout, ( remaining + EHCI_ISO_COMPLETE_GRACE + 8 ) * 125 );
				}

#ifdef DO_DEBUG

				if ( ioreq->req_TimerAdded )
				{
					USBPANIC ( "HCD_myRequest_Add : Error TimerAdded == TRUE ( ioreq %p )", ioreq );
				}
				else

#endif

					MEM_COPY ( usbbase->usb_TimeRequest, &ioreq->req_TimerIOReq, sizeof ( struct TimeRequest ) );

				ioreq->req_TimerIOReq.Request.io_Message.mn_Node.ln_Name = (PTR)ioreq;
				ioreq->req_TimerIOReq.Request.io_Message.mn_ReplyPort	 = &hn->hn_WatchDog_MsgPort.ump_MsgPort;
				ioreq->req_TimerIOReq.Request.io_Command				 = TR_ADDREQUEST;
				ioreq->req_TimerIOReq.Time.Microseconds					 = timeout % 1000000;
				ioreq->req_TimerIOReq.Time.Seconds						 = timeout / 1000000;
				ioreq->req_TimerAdded									 = TRUE;

				IO_SEND ( &ioreq->req_TimerIOReq );
			}
		}
		else
		{
			// --
			// Something wrong, Free resources

			if ( hn->HCD_Functions.Transfer_Free )
			{
				hn->HCD_Functions.Transfer_Free ( hn, ioreq );
			}
			else
			{
				USBERROR ( "HCD_myRequest_Add : Transfer_Free : Not Implemented in HCD" );
			}
		}
	}

	// --
	// Set error code, if not allready set

	if ( ! ioreq->req_Public.io_Error )
	{
		ioreq->req_Public.io_Error = err;
	}

	return ( handled );
}

// --

SEC_CODE S32
__HCD_Request_CanStart ( struct USB2_HCDNode * hn, struct RealRequest * ioreq )
{
	struct USB2_EndPointNode * ep;
	struct RealRequest *	   active;

	U32 budget;
	U32 step;
	U32 count;
	U32 speed;
	U32 command;

	ep	   = ioreq->req_EndPoint;
	active = ep->ep_Request_Active;

	if ( ! active )
	{
		return ( TRUE );
	}

	speed	= ioreq->req_Function->fkt_Speed;
	command = ioreq->req_Public.io_Command;

	// Keep one batch ahead when the driver supplies individual packet results.
	if ( ( ep->ep_Request_ActiveNext ) || ( ! hn->hn_IsEHCI ) || ( ep->ep_Type != EPATT_Type_Isochronous )
		 || ( active->req_Public.io_Command != command )
		 || ( ! ( ( ( speed == USBSPEED_High ) && ( command == CMD_READ ) )
				  || ( ( speed == USBSPEED_Full ) && ( command == CMD_WRITE ) ) ) )
		 || ( ! active->req_Public.io_IsoPacketLengths ) || ( ! ioreq->req_Public.io_IsoPacketLengths ) || ( ! ep->ep_Interval )
		 || ( ep->ep_Interval > 16 ) )
	{
		return ( FALSE );
	}

	budget = hn->hn_HCD.EHCI.FrameList_Size * 8 - EHCI_ISO_RETRY_LEAD - 8;
	step   = 1U << ( ep->ep_Interval - 1 );

	if ( speed == USBSPEED_Full )
	{
		step *= 8;
	}

	count = ioreq->req_Public.io_IsoPacketCount;

	return ( ( count ) && ( count <= EHCI_ISO_MAX_PACKETS )
			 && ( active->req_Public.io_IsoPacketCount + count <= budget / step ) );
}

// --

/*
\\
// Returns:
\\   TRUE  - if Transfer was handled and is finish.
//   FALSE - if Transfer need more time to finish
\\
*/

SEC_CODE S32
__HCD_Add_Request ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct RealRequest * ioreq )
{
	struct USB2_EndPointNode * ep;
	S32						   handled;
	// U32 val;

	TASK_NAME_ENTER ( "HCD : __HCD_Add_Request" );

	ep = ioreq->req_EndPoint;

	// --
	// Control check : Need a timeout and max 5 sec

	if ( ep->ep_Type == EPATT_Type_Control )
	{
		/**/ if ( ! ioreq->req_Public.io_TimeOut )
		{
			USBINFO ( "Control IOReq needs a timeout, setting 5sec" );
			ioreq->req_Public.io_TimeOut = 5 * 1000000;
		}
		else if ( ioreq->req_Public.io_TimeOut > 5 * 1000000 )
		{
			USBINFO ( "Control IOReq : Clamping to 5sec timeout" );
			ioreq->req_Public.io_TimeOut = 5 * 1000000;
		}
	}

	// Can we start Request or do we queue it
	if ( ( ( ep->ep_Request_Queue.uh_Head ) && ( ioreq->req_PublicStat != IORS_HCD_Queued ) )
		 || ( ! __HCD_Request_CanStart ( hn, ioreq ) ) )
	{
		USBDEBUG ( "Queuing Node for Addr $%02ld : EP    $%02lx", ioreq->req_Function->fkt_Address, ep->ep_Number );

		ioreq->req_PublicStat = IORS_HCD_Queued;
		NODE_ADDTAIL ( &ep->ep_Request_Queue, ioreq );

		handled = FALSE;
	}
	else
	{
		handled = myAdd_Request ( usbbase, hn, ioreq );

		if ( ( handled ) && ( ep->ep_Request_Active ) && ( ioreq->req_Public.io_Error == USB2Err_Host_NoBandWidth ) )
		{
			// The future slots are busy. Let the request try again when the active batch ends.
			ioreq->req_Public.io_Error = USB2Err_NoError;
			ioreq->req_PublicStat	   = IORS_HCD_Queued;

			NODE_ADDHEAD ( &ep->ep_Request_Queue, ioreq );

			handled = FALSE;
		}
	}

	TASK_NAME_LEAVE ();

	return ( handled );
}

// --
