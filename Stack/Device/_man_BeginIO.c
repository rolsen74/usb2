
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

static S32
AllowControlStateChange ( struct RealRequest * ioreq )
{
	struct USB2_SetupData * setup;

	setup = ioreq->req_Public.io_SetupData;

	if ( ( setup->RequestCode == REQCODE_Set_Interface )
		 && ( setup->RequestType == ( REQTYPE_Write | REQTYPE_Standard | REQTYPE_Interface ) ) )
	{
		return ( ioreq->req_AltHeader != NULL );
	}

	if ( ( setup->RequestCode == REQCODE_Set_Configuration )
		 && ( setup->RequestType == ( REQTYPE_Write | REQTYPE_Standard | REQTYPE_Device ) ) )
	{
		return ( ioreq->req_SetConfig );
	}

	return ( TRUE );
}

// --

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

SEC_CODE static S32
ValidControlRequest ( const struct USB2_IORequest * request )
{
	const struct USB2_SetupData * setup;

	setup = request->io_SetupData;

	if ( ( ! setup ) || ( request->io_SetupLength != sizeof ( *setup ) )
		 || ( request->io_Length != LE_SWAP16 ( setup->Length ) ) )
	{
		return ( FALSE );
	}

	if ( request->io_Length )
	{
		return ( ( request->io_Data ) &&
			( ( request->io_Command == CMD_READ ) == ( ( setup->RequestType & REQTYPE_Read ) != 0 ) ) );
	}

	return ( TRUE );
}

// --

// Called with the stack lock held.
static S32
PrepareHaltClear ( struct USBBase * usbbase, struct RealRequest * ioreq, struct USB2_HCDNode * hn )
{
	struct USB2_SetupData *		setup;
	struct USB2_EndPointNode *	target;
	struct USB2_InterfaceNode * in;
	struct RealEndPointResource * resource;

	U32 address;
	U32 slot;

	setup = ioreq->req_Public.io_SetupData;

	if ( ( ioreq->req_Public.io_Command != CMD_WRITE ) || ( ioreq->req_EndPoint->ep_Type != EPATT_Type_Control )
		 || ( ioreq->req_Public.io_Data ) || ( ioreq->req_Public.io_Length ) || ( ! setup )
		 || ( ioreq->req_Public.io_SetupLength != sizeof ( *setup ) )
		 || ( setup->RequestType != ( REQTYPE_Write | REQTYPE_Standard | REQTYPE_EndPoint ) )
		 || ( setup->RequestCode != REQCODE_Clear_Feature ) || ( setup->Value != LE_SWAP16 ( USBFEATURE_EndPoint_Halt ) ) )
	{
		return ( TRUE );
	}

	address = LE_SWAP16 ( setup->Index );

	if ( ( address & ~( EPADR_Dir_Mask | EPADR_EndPoint_Mask ) ) || ( ioreq->req_Function->fkt_Address >= 128 ) )
	{
		return ( FALSE );
	}

	slot = ( ioreq->req_Function->fkt_Address * 32 ) + EPADR_Dir_Number ( address );

	if ( ( address & EPADR_Dir_Mask ) == EPADR_Dir_Out )
	{
		slot += 16;
	}

	target = hn->hn_EndPoint_Array[slot];

	if ( ( ! target ) || ( target->ep_Function != ioreq->req_Function ) || ( ! target->ep_Active ) )
	{
		return ( FALSE );
	}

	in = target->ep_Interface;
	resource = ioreq->req_Resource;

	if ( ( in ) && ( ( in->in_Parent->ih_Changing ) || ( in->in_Parent->ih_Active != in )
		 || ( ( resource ) && ( in->in_Parent->ih_Owner != resource->epr_Register )
			  && ( ioreq->req_Function->fkt_Owner != resource->epr_Register ) ) ) )
	{
		return ( FALSE );
	}

	if ( ioreq->req_DestallTarget )
	{
		return ( ioreq->req_DestallTarget == target );
	}

	if ( ENDPOINT_LOCK ( target ) != LSTAT_Okay )
	{
		return ( FALSE );
	}

	ioreq->req_DestallTarget = target;

	return ( TRUE );
}

// --

static void
_manager_BeginIO ( struct DeviceManagerInterface * Self, struct IORequest * request )
{
	struct RealEndPointResource * resource;
	struct RealFunctionNode *	  fn;
	struct USB2_InterfaceNode *	  in;
	struct USB2_HCDNode *		  hn;
	struct RealRequest *		  ioreq;
	struct USBBase *			  usbbase;

	usbbase = (PTR)Self->Data.LibBase;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	ioreq = FindLiveRequest ( usbbase, request );

	if ( ( ioreq ) && ( ioreq->req_PublicStat != IORS_User ) )
	{
		// The outstanding submission owns the message and its result.
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		return;
	}

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		NSCmd_DeviceQuery ( (PTR)request );
	}
	else
	{
		if ( ! ioreq )
		{
			request->io_Error = IOERR_NOCMD;
		}
		else
		{
			fn							= ioreq->req_Function;
			hn							= fn->fkt_HCD;
			ioreq->req_Public.io_Actual = 0;

			if ( ( request->io_Command == CMD_WRITE ) && ( ioreq->req_EndPoint->ep_Type == EPATT_Type_Isochronous ) &&
				( ioreq->req_Public.io_IsoPacketLengths ) )
			{
				ioreq->req_Public.io_Offset = 0;
			}

			in		 = ioreq->req_EndPoint->ep_Interface;
			resource = ioreq->req_Resource;

			if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( fn->fkt_PortRecovery ) || ( ioreq->req_FreeMe ) || ( ioreq->req_Detach )
				 || ( ( resource ) && ( ( resource->epr_FreeMe ) || ( resource->epr_Register->reg_FreeMe ) ) ) )
			{
				request->io_Error = USB2Err_Device_Detached;
			}
			else if ( ( ! in ) && ( resource ) && ( ! Register_HasControlClaim ( resource->epr_Register, NULL ) ) )
			{
				request->io_Error = USB2Err_Stack_EndPointNotFound;
			}
			else if ( ( in ) && ( ( in->in_Parent->ih_Changing ) || ( in->in_Parent->ih_Active != in )
					 || ( ( resource ) && ( in->in_Parent->ih_Owner != resource->epr_Register )
						  && ( fn->fkt_Owner != resource->epr_Register ) ) ) )
			{
				request->io_Error = USB2Err_Stack_EndPointNotFound;
			}

			else if ( HCD_VALID ( hn ) != VSTAT_Okay )
			{
				request->io_Error = USB2Err_Stack_InvalidStructure;
			}
			else if ( ( ( ! hn->hn_AcceptIO ) && ( ( fn->fkt_Tier ) || ( ! hn->hn_RootHubRecovering ) ||
					( request->io_Command == CMD_WRITE ) ) ) ||
				 ( ( ioreq->req_CheckEpoch ) && ( ioreq->req_ExpectedEpoch != hn->hn_IOEpoch ) ) ||
				 ( ( fn->fkt_EnumFenced ) && ( fn->fkt_EnumEpoch != hn->hn_IOEpoch ) ) ||
				 ( ( fn->fkt_Tier ) && ( fn->fkt_HCDGeneration != hn->hn_Generation ) ) )
			{
				request->io_Error = USB2Err_Device_Detached;
			}
			else if ( ( ioreq->req_EndPoint->ep_Type == EPATT_Type_Control )
				 && ( ( request->io_Command == CMD_READ ) || ( request->io_Command == CMD_WRITE ) )
				 && ( ! ValidControlRequest ( &ioreq->req_Public ) ) )
			{
				request->io_Error = USB2Err_Stack_InvalidStructure;
			}
			else if ( ( ioreq->req_EndPoint->ep_Type == EPATT_Type_Control )
				 && ( ( request->io_Command == CMD_READ ) || ( request->io_Command == CMD_WRITE ) )
				 && ( ! AllowControlStateChange ( ioreq ) ) )
			{
				request->io_Error = IOERR_NOCMD;
			}
			else if ( ! PrepareHaltClear ( usbbase, ioreq, hn ) )
			{
				request->io_Error = USB2Err_Stack_EndPointNotFound;
			}
			else
			{
				request->io_Error = 0;
				request->io_Flags &= ~IOF_QUICK;
				ioreq->req_Submission++;
				ioreq->req_SubmitEpoch = hn->hn_IOEpoch;
				ioreq->req_AbortRequested = FALSE;
				ioreq->req_PublicStat	  = IORS_HCD_Pending;

				MSGPORT_PUTMSG ( &hn->hn_Begin_MsgPort, ioreq );

				SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

				return;
			}

			if ( usbbase->usb_Notify_IOWatchers )
			{
				USB2_Notify_IORequest ( usbbase, ioreq );
			}
		}
	}

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		MSGPORT_REPLYMSG ( request );
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
}
