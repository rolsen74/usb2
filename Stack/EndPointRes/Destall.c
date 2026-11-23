
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE U32
__EndPointRes_Destall ( struct USBBase * usbbase, struct USB2_EPResource * epr, STR file UNUSED )

#else

SEC_CODE U32
__EndPointRes_Destall ( struct USBBase * usbbase, struct USB2_EPResource * epr )

#endif

{
	struct RealEndPointResource * real;
	struct USB2_EndPointNode *	  ep;
	struct RealFunctionNode *	  fn;
	struct USB2_InterfaceNode *	  in;
	struct USB2_SetupData		  sd;
	struct RealRequest *		  ioreq;

	U32 retval;
	U32 locked;
	U32 idx;

	TASK_NAME_ENTER ( "__EndPointRes_Destall" );

	//	USBERROR( "__EndPointRes_Destall       : EPR   %p : (%s)", epr, (file)?file:"<NULL>" );

	retval = FALSE;
	locked = FALSE;
	real   = NULL;

	if ( ! epr )
	{
		USBERROR ( "__EndPointRes_Destall : NULL Pointer" );
		goto bailout;
	}

	real = (PTR)( (char *)epr - offsetof ( struct RealEndPointResource, epr_Public ) );

	if ( ENDPOINTRES_LOCK ( real ) != LSTAT_Okay )
	{
		USBERROR ( "__EndPointRes_Destall : Error invalid EndPoint Resource (%p)", epr );
		goto bailout;
	}

	locked = TRUE;

	if ( real->epr_Detach )
	{
		USBERROR ( "__EndPointRes_Destall : EndPoint detached" );
		goto bailout;
	}

	ep = real->epr_EndPoint;

	if ( ENDPOINT_VALID ( ep ) != VSTAT_Okay )
	{
		USBERROR ( "__EndPointRes_Destall : Error validing EndPoint Node" );
		goto bailout;
	}

	fn = ep->ep_Function;

	if ( FUNCTION_VALID ( fn ) != VSTAT_Okay )
	{
		USBERROR ( "__EndPointRes_Destall : Error validing Function Node" );
		goto bailout;
	}

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	in = ep->ep_Interface;

	if ( ( real->epr_FreeMe ) || ( real->epr_Register->reg_FreeMe ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe )
		 || ( fn->fkt_PortRecovery ) || ( ! ep->ep_Active )
		 || ( ( in ) && ( ( in->in_Parent->ih_Changing ) || ( in->in_Parent->ih_Active != in )
			  || ( ( in->in_Parent->ih_Owner != real->epr_Register ) && ( fn->fkt_Owner != real->epr_Register ) ) ) ) )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		goto bailout;
	}

	if ( ep->ep_Type == EPATT_Type_Control )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		retval = TRUE;
		goto bailout;
	}

	ioreq = IOREQUEST_ALLOCTAGS ( USB2Tag_IOReq_EndPoint, fn->fkt_ControlEndPoint, USB2Tag_IOReq_TimeOut, 500000, TAG_END );

	if ( ( ioreq ) && ( ENDPOINT_LOCK ( ep ) == LSTAT_Okay ) )
	{
		ioreq->req_DestallTarget = ep;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( ! ioreq )
	{
		USBERROR ( "__EndPointRes_Destall : Error creating IORequest" );
		goto bailout;
	}

	if ( ! ioreq->req_DestallTarget )
	{
		IOREQUEST_FREE ( ioreq );

		goto bailout;
	}

	idx = ( ep->ep_Direction == EPDIR_Out ) ? EPADR_Dir_Out : EPADR_Dir_In;
	idx |= ( ep->ep_Number );

	sd.RequestType = REQTYPE_Write | REQTYPE_Standard | REQTYPE_EndPoint;
	sd.RequestCode = REQCODE_Clear_Feature;
	sd.Value	   = LE_SWAP16 ( USBFEATURE_EndPoint_Halt );
	sd.Index	   = LE_SWAP16 ( idx );
	sd.Length	   = LE_SWAP16 ( 0 );

	ioreq->req_Public.io_Command	 = CMD_WRITE;
	ioreq->req_Public.io_Data		 = NULL;
	ioreq->req_Public.io_Length		 = 0;
	ioreq->req_Public.io_SetupData	 = &sd;
	ioreq->req_Public.io_SetupLength = sizeof ( struct USB2_SetupData );

	//	MSGPORT_PUTMSG( & fkt->fkt_HCDNode->hn_Begin_MsgPort, ioreq );
	//	MSGPORT_WAIT(   (APTR) ioreq->rr_IOReq.io_Message.mn_ReplyPort );
	//	MSGPORT_GETMSG( (APTR) ioreq->rr_IOReq.io_Message.mn_ReplyPort );

	if ( ! IO_DO ( ioreq ) )
	{
		retval = TRUE;
	}
	else
	{
		USBERROR ( "__EndPointRes_Destall : Failed io_Error = %ld", ioreq->req_Public.io_Error );
	}

	IOREQUEST_FREE ( ioreq );

bailout:

	if ( locked )
	{
		ENDPOINTRES_UNLOCK ( real );
	}

	TASK_NAME_LEAVE ();
	return ( retval );
}

// --
