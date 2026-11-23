
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

#include "NSCmd_DeviceQuery.c"

// --

SEC_CODE void
IOREQUEST_PRINT ( struct USBBase * usbbase, struct RealRequest * ioreq )
{
	struct ExecIFace * IExec;

	IExec = usbbase->usb_IExec;

	IExec->Disable ();
	IExec->DebugPrintF ( "IORequest : ioreq %p\n", ioreq );
	IExec->DebugPrintF ( "req_StructID ..... : $%08lx\n", ioreq->req_StructID );
	IExec->DebugPrintF ( "req_Locks ........ : %ld\n", ioreq->req_Locks );
	IExec->DebugPrintF ( "req_Detach ....... : %ld\n", ioreq->req_Detach );
	IExec->DebugPrintF ( "req_Function ..... : %p\n", ioreq->req_Function );
	IExec->DebugPrintF ( "req_EndPoint ..... : %p\n", ioreq->req_EndPoint );
	IExec->DebugPrintF ( "req_PublicStat .... : %ld\n", (S32)ioreq->req_PublicStat );
	IExec->Enable ();

#if 0
	struct USB2_IORequest			req_Public;
	// -- 
	U32								req_StructID;
	S32								req_Locks;
	U16								req_Detach;
	U16								req_FreeMe;
//	struct USB2_TaskNode *			req_Task;
//	struct USB2_ASync *				req_ASync;
	// --
	struct RealFunctionNode *		req_Function;
	struct USB2_EndPointNode *		req_EndPoint;
	struct USB2_MsgPort				req_MsgPort;
//	struct TimeRequest				req_TimerIOReq;
	enum IORStat					req_PublicStat;
#endif
}

// Called with the stack lock held.
static struct RealRequest *
FindLiveRequest ( struct USBBase * usbbase, const struct IORequest * request )
{
	struct USB2_Node * node;

	for ( node = usbbase->usb_IORequests.uh_Head; node; node = Node_Next ( node ) )
	{
		if ( node->un_Data == (const void *)request )
		{
			return ( node->un_Data );
		}
	}

	return ( NULL );
}

// --

static void
_manager_BeginIO ( struct DeviceManagerInterface * Self, struct IORequest * request )
{
	struct RealFunctionNode *	fn;
	struct USB2_InterfaceNode * in;
	struct USB2_HCDNode *		hn;
	struct RealRequest *		ioreq;
	struct USBBase *			usbbase;

	usbbase = (PTR)Self->Data.LibBase;

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		if ( request->io_Message.mn_Length >= sizeof ( struct IOStdReq ) )
		{
			( (struct IOStdReq *)request )->io_Actual = 0;

			NSCmd_DeviceQuery ( usbbase, (PTR)request );
		}
		else
		{
			request->io_Error = IOERR_BADLENGTH;
		}
	}
	else
	{
		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		ioreq = FindLiveRequest ( usbbase, request );

		if ( ! ioreq )
		{
			request->io_Error = IOERR_NOCMD;
		}
		else if ( ioreq->req_PublicStat != IORS_User )
		{
			// The outstanding submission owns the message and its result.
			SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

			return;
		}
		else
		{
			fn							= ioreq->req_Function;
			hn							= fn->fkt_HCD;
			ioreq->req_Public.io_Actual = 0;

			in = ioreq->req_EndPoint->ep_Interface;

			if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( ioreq->req_FreeMe ) || ( ioreq->req_Detach ) )
			{
				request->io_Error = USB2Err_Device_Detached;
			}
			else if ( ( in ) && ( in->in_Parent->ih_Changing ) )
			{
				request->io_Error = USB2Err_Stack_EndPointNotFound;
			}

			else if ( HCD_VALID ( hn ) != VSTAT_Okay )
			{
				request->io_Error = USB2Err_Stack_InvalidStructure;
			}
			else
			{
				request->io_Error = 0;
				request->io_Flags &= ~IOF_QUICK;
				ioreq->req_Submission++;
				ioreq->req_AbortRequested = FALSE;
				ioreq->req_PublicStat	  = IORS_HCD_Pending;

				MSGPORT_PUTMSG ( &hn->hn_Begin_MsgPort, ioreq );

				SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

				return;
			}
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
	}

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		MSGPORT_REPLYMSG ( request );
	}
}
