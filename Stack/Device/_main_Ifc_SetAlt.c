/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

static struct USB2_InterfaceNode *
FindAlternate ( struct USB2_InterfaceHeader * ih, U32 alternate )
{
	struct USB2_InterfaceNode * in;

	for ( in = ih->ih_AltSettings.uh_Head; in; in = Node_Next ( in ) )
	{
		if ( in->in_AltNumber == alternate )
		{
			break;
		}
	}

	return ( in );
}

// --

static S32
_main_Ifc_SetAlt ( struct USB2IFace * Self, struct USB2_Register * pubreg, struct USB2_Interface * ifc, U32 alt )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceNode *	  in;
	struct USB2_InterfaceNode *	  old;
	struct USB2_EndPointNode *	  ep;
	struct USB2_EndPointNode **	  array;
	struct USB2_Node *			  node;
	struct RealRequest *		  pending;
	struct RealRequest *		  ioreq;
	struct USB2_SetupData		  setup;
	struct RealFunctionNode *	  fn;
	struct RealRegister *		  reg;
	struct USBBase *			  usbbase;

	U32 offset;
	U32 slot;
	S32 result;
	S32 applied;

	if ( ( ! pubreg ) || ( ! ifc ) || ( alt > 255 ) )
	{
		return ( FALSE );
	}

	reg		= (PTR)pubreg;
	usbbase = (PTR)Self->Data.LibBase;
	result	= FALSE;

	if ( REGISTER_LOCK ( reg ) != LSTAT_Okay )
	{
		return ( FALSE );
	}

	ih = LockRegistrationInterface ( usbbase, reg, ifc );

	if ( ! ih )
	{
		REGISTER_UNLOCK ( reg );

		return ( FALSE );
	}

	fn = ih->ih_Function;

	SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( ih->ih_Owner != reg ) || ( ih->ih_Changing ) )
	{
		goto unlock;
	}

	in = FindAlternate ( ih, alt );

	if ( ! in )
	{
		goto unlock;
	}

	if ( ih->ih_Active == in )
	{
		result = TRUE;
		goto unlock;
	}

	old = ih->ih_Active;

	for ( node = usbbase->usb_IORequests.uh_Head; node; node = Node_Next ( node ) )
	{
		pending = node->un_Data;

		if ( ( old ) && ( pending->req_Function == fn ) && ( pending->req_EndPoint->ep_Interface == old )
			 && ( pending->req_PublicStat != IORS_User ) )
		{
			goto unlock;
		}
	}

	ioreq = IOREQUEST_ALLOCTAGS ( USB2Tag_IOReq_EndPoint, fn->fkt_ControlEndPoint, USB2Tag_IOReq_TimeOut, 5000000, TAG_END );

	if ( ! ioreq )
	{
		goto unlock;
	}

	ih->ih_Changing		 = TRUE;
	ioreq->req_AltHeader = ih;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

	setup.RequestType				 = REQTYPE_Write | REQTYPE_Standard | REQTYPE_Interface;
	setup.RequestCode				 = REQCODE_Set_Interface;
	setup.Value						 = LE_SWAP16 ( alt );
	setup.Index						 = LE_SWAP16 ( ih->ih_Number );
	setup.Length					 = 0;
	ioreq->req_Public.io_Command	 = CMD_WRITE;
	ioreq->req_Public.io_SetupData	 = &setup;
	ioreq->req_Public.io_SetupLength = sizeof ( setup );

	IO_DO ( ioreq );

	result	= ioreq->req_Public.io_Error == USB2Err_NoError;
	applied = ( ioreq->req_AltApplied ) || ( ioreq->req_Public.io_Error == USB2Err_Host_Timeout );

	IOREQUEST_FREE ( ioreq );

	SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	if ( ( ! result ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( ih->ih_Owner != reg ) )
	{
		// A device-side switch without a matching controller state requires re-enumeration.
		ih->ih_Changing = applied;
		result			= FALSE;
		goto unlock;
	}

	in = FindAlternate ( ih, alt );

	if ( ! in )
	{
		result = FALSE;
		goto unlock;
	}

	old	   = ih->ih_Active;
	array  = fn->fkt_HCD->hn_EndPoint_Array;
	offset = fn->fkt_Address * 32;
	ep	   = ( old ) ? old->in_EndPoints.uh_Head : NULL;

	while ( ep )
	{
		ep->ep_Active = FALSE;
		slot		  = offset + ep->ep_Number + ( ( ep->ep_Direction == EPDIR_Out ) ? 16 : 0 );

		if ( array[slot] == ep )
		{
			array[slot] = NULL;
		}

		ep = Node_Next ( ep );
	}

	ih->ih_Active	 = in;
	ih->ih_AltNumber = alt;
	ep				 = in->in_EndPoints.uh_Head;

	while ( ep )
	{
		slot			  = offset + ep->ep_Number + ( ( ep->ep_Direction == EPDIR_Out ) ? 16 : 0 );
		array[slot]		  = ep;
		ep->ep_Active	  = TRUE;
		ep->ep_DataToggle = 0;
		ep->ep_Stalled	  = FALSE;
		ep->ep_PingState  = 0;
		ep				  = Node_Next ( ep );
	}

	ih->ih_Changing = FALSE;

	result = TRUE;

unlock:

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

	INTERFACE_UNLOCKHEADER ( ih );
	REGISTER_UNLOCK ( reg );

	return ( result );
}

// --
