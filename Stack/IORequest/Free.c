
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --
// Remove this request's reply without consuming other messages on the port.
SEC_CODE static void
CollectReply ( struct USBBase * usbbase, struct RealRequest * ioreq )
{
	struct ExecIFace * IExec;
	struct MsgPort *   port;
	struct Node *	   node;

	port = ioreq->req_Public.io_Message.mn_ReplyPort;

	if ( ! port )
	{
		return;
	}

	IExec = usbbase->usb_IExec;

	IExec->Disable ();

	for ( node = port->mp_MsgList.lh_Head; node->ln_Succ; node = node->ln_Succ )
	{
		if ( node == &ioreq->req_Public.io_Message.mn_Node )
		{
			IExec->Remove ( node );

			break;
		}
	}

	IExec->Enable ();
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE enum FSTAT
__IORequest_Free ( struct USBBase * usbbase, struct RealRequest * ioreq, STR file UNUSED )

#else

SEC_CODE enum FSTAT
__IORequest_Free ( struct USBBase * usbbase, struct RealRequest * ioreq )

#endif

{
	enum VSTAT vstat;
	enum FSTAT fstat;

	TASK_NAME_ENTER ( "__IORequest_Free" );

	USBDEBUG ( "__IORequest_Free         : IOReq %p : (%s)", ioreq, ( file ) ? file : "<NULL>" );

	// --

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	vstat = IOREQUEST_VALID ( ioreq );

	/**/ if ( vstat == VSTAT_Null )
	{
		fstat = FSTAT_Null;
	}
	else if ( ( vstat == VSTAT_Okay ) && ( ioreq->req_MsgPort.ump_StructID == ID_USB2_MP )
			  && ( ioreq->req_MsgPort.ump_MsgPort.mp_SigTask != TASK_FIND () ) )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : free request on its reply-port task\n" );

		fstat = FSTAT_Error;
	}
	else if ( vstat == VSTAT_Okay )
	{
		ioreq->req_FreeMe = TRUE;

		//		usbbase->usb_IExec->DebugPrintF( "__IORequest_Free         : IOReq %p : Locks %2ld : Stat %ld : Task %p :\n",
		// ioreq, ioreq->req_Locks, ioreq->req_PublicStat, usbbase->usb_IExec->FindTask(NULL) );

		if ( ( ioreq->req_PublicStat != IORS_User ) && ( ioreq->req_PublicStat != IORS_Unset ) )
		{
			usbbase->usb_IExec->DebugPrintF ( "__IORequest_Free         : IOReq %p : Node still active (Stat %ld)\n", ioreq,
											  ioreq->req_PublicStat );
			USBDEBUG ( "__IORequest_Free         : IOReq %p : Node still active (Stat %ld)", ioreq, ioreq->req_PublicStat );
			fstat = FSTAT_Locked;
		}
		else if ( ioreq->req_Locks > 0 )
		{
			usbbase->usb_IExec->DebugPrintF ( "__IORequest_Free         : IOReq %p : Node still have %ld locks\n", ioreq,
											  ioreq->req_Locks );
			USBDEBUG ( "__IORequest_Free         : IOReq %p : Node still have %ld locks", ioreq, ioreq->req_Locks );
			fstat = FSTAT_Locked;
		}
		else
		{
			if ( ioreq->req_LiveNode.un_Data )
			{
				NODE_REMNODE ( &usbbase->usb_IORequests, &ioreq->req_LiveNode );
			}

			CollectReply ( usbbase, ioreq );

			ioreq->req_StructID = ID_USB2_FREED;
			fstat				= FSTAT_Okay;
		}
	}
	else // ( vstat == VSTAT_Error )
	{
		USBPANIC ( "__IORequest_Free    : Invalid Pointer (%p)", ioreq );
		fstat = FSTAT_Error;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	// --

	if ( fstat != FSTAT_Okay )
	{
		goto bailout;
	}

	USBINFO ( "__IORequest_Free         : Freeing %p : (%s)", ioreq, ( file ) ? file : "<NULL>" );

	// --

	//	#ifdef DO_PANIC
	//
	//	Okay this is used in a MsgPort.. so it do not get Removed via Node_xx
	//	if ((( Node_Next(ioreq)) && ( Node_Next(ioreq) != (PTR) 0xAC111111 ))
	//	||	(( Node_Prev(ioreq)) && ( Node_Prev(ioreq) != (PTR) 0xAC222222 )))
	//	{
	//		USBPANIC( "__IORequest_Free    : Register Node have not been removed : Self %p : Next %p : Prev %p : (%s)", ioreq,
	// Node_Next(ioreq), Node_Prev(ioreq), file );
	//	}
	//
	//	#endif

	// --

#if 0

	if ( ioreq->rr_StructID != StructID_IOR )
	{
		USBDEBUG( "Invalid Request ID" );
		goto bailout;
	}

#ifdef DO_DEBUG
	if (( ioreq->rr_HCDActive ) || ( ioreq->rr_HCDControlled ))
	{
		USBDEBUG( "IORequest is still Active" );
	}

	if ( ioreq->rr_TimerAdded )
	{
		USBDEBUG( "IORequest's Timer is still Active" );
	}
#endif

	if (( ioreq->rr_MsgPortUsed ) && ( ioreq->rr_IOReq.io_Message.mn_ReplyPort ))
	{
		MSGPORT_FREE( & ioreq->rr_MsgPort );
		ioreq->rr_IOReq.io_Message.mn_ReplyPort = NULL;
		ioreq->rr_MsgPortUsed = FALSE;
	}

#endif

	if ( ioreq->req_DestallTarget )
	{
		ENDPOINT_UNLOCK ( ioreq->req_DestallTarget );
	}

	if ( ioreq->req_EndPoint )
	{
		ENDPOINT_UNLOCK ( ioreq->req_EndPoint );
		ioreq->req_EndPoint = NULL;
	}

	if ( ioreq->req_Function )
	{
		FUNCTION_UNLOCK ( ioreq->req_Function );
		ioreq->req_Function = NULL;
	}

	//	usbbase->usb_IExec->DebugPrintF( "FreeMP %p\n", & ioreq->req_MsgPort.ump_MsgPort );

	MSGPORT_FREE ( &ioreq->req_MsgPort );

	MEMORY_FREE ( MEMID_USBIORequest, ioreq, 0 );

bailout:

	TASK_NAME_LEAVE ();

	return ( fstat );
}
