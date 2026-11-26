
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

static S32 Send_HCDs ( struct USBBase * usbbase, struct List * snapshot );
static S32 Send_Functions ( struct USBBase * usbbase, struct List * snapshot );
static S32 Send_Interfaces ( struct USBBase * usbbase, struct List * snapshot );

// --

SEC_CODE static PTR
_main_Notify_Add ( struct USB2IFace * Self, U32 type, struct MsgPort * mp )
{
	struct USB2_NotifyNode *	node;
	struct USBBase *			usbbase;
	struct USB2_NotifyMessage * msg;
	struct List					snapshot;

	S32 okay;

	usbbase = (PTR)Self->Data.LibBase;

	node = NULL;

	if ( type >= USBNT_Last )
	{
		USBERROR ( "Error unsuported Notify type (%ld)", type );
		goto bailout;
	}

	if ( ! mp )
	{
		USBERROR ( "Error MsgPort pointer is NULL" );
		goto bailout;
	}

	node = MEMORY_ALLOC ( MEMID_NotifyNode, TRUE );

	if ( ! node )
	{
		USBERROR ( "Error allocating memory" );
		goto bailout;
	}

	node->nn_StructID = ID_USB2_NN;
	node->nn_MsgPort  = mp;
	node->nn_Type	  = type;

	usbbase->usb_IExec->NewList ( &snapshot );

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_Notify_Semaphore );

	okay = FALSE;

	if ( usbbase->usb_Notify_ReplyMsgPort )
	{
		switch ( type )
		{
			case USBNT_HCD:
				okay = Send_HCDs ( usbbase, &snapshot );
				break;
			case USBNT_Function:
				okay = Send_Functions ( usbbase, &snapshot );
				break;
			case USBNT_Interface:
				okay = Send_Interfaces ( usbbase, &snapshot );
				break;
			case USBNT_IORequest:
				okay = TRUE;
				break;
		}
	}

	if ( okay )
	{
		NODE_ADDTAIL ( &usbbase->usb_Notify_Header, node );
	}

	while ( ( msg = (PTR)usbbase->usb_IExec->RemHead ( &snapshot ) ) )
	{
		if ( okay )
		{
			usbbase->usb_IExec->PutMsg ( mp, &msg->nm_Message );
		}
		else
		{
			MEMORY_FREE ( MEMID_NotifyMessage, msg, 0 );
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_Notify_Semaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );

	if ( ! okay )
	{
		MEMORY_FREE ( MEMID_NotifyNode, node, 0 );

		node = NULL;
	}

bailout:

	return ( node );
}

// --

static S32
Send_HCDs ( struct USBBase * usbbase, struct List * snapshot )
{
	struct USB2_NotifyMessage * msg;
	struct USB2_HCDNode *		hcd;

	// HCD is a Static list build at Startup
	// so no locking is nesary.

	hcd = usbbase->usb_HCDHeader.uh_Head;

	while ( hcd )
	{
		if ( hcd->hn_Task )
		{
			msg = MEMORY_ALLOC ( MEMID_NotifyMessage, TRUE );

			if ( ! msg )
			{
				return ( FALSE );
			}

			// Default Notify Data

			msg->nm_Message.mn_ReplyPort = &usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
			msg->nm_Message.mn_Length	 = sizeof ( struct USB2_NotifyMessage );
			msg->nm_Type				 = USBNT_HCD;
			msg->nm_Command				 = USBNC_Adding;
			msg->nm_ID					 = hcd->hn_NotifyID;

			// Function Specific Info

			msg->nm_Data.nm_HCD.nm_Type	 = hcd->hn_HCDType;
			msg->nm_Data.nm_HCD.nm_Index = hcd->hn_HCDIndex;

			usbbase->usb_IExec->AddTail ( snapshot, &msg->nm_Message.mn_Node );
		}

		hcd = Node_Next ( hcd );
	}

	return ( TRUE );
}

// --

static S32
Send_Functions ( struct USBBase * usbbase, struct List * snapshot )
{
	struct USB2_NotifyMessage * msg;
	struct RealFunctionNode *	fn;
	struct USB2_Node *			n;
	U32							hcdid;

	fn = NULL;

	n = usbbase->usb_Fkt_Header.uh_Tail;

	while ( n )
	{
		fn = n->un_Data;

		if ( ( fn ) && ( fn->fkt_Initialized ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) )
		{
			msg = MEMORY_ALLOC ( MEMID_NotifyMessage, TRUE );

			if ( ! msg )
			{
				return ( FALSE );
			}

			// Default Notify Data

			msg->nm_Message.mn_ReplyPort = &usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
			msg->nm_Message.mn_Length	 = sizeof ( struct USB2_NotifyMessage );
			msg->nm_Type				 = USBNT_Function;
			msg->nm_Command				 = USBNC_Adding;
			msg->nm_ID					 = fn->fkt_NotifyID;

			// Function Specific Info

			hcdid = fn->fkt_HCD->hn_NotifyID;

			msg->nm_Data.nm_Function.nm_HCD_ID	  = hcdid;
			msg->nm_Data.nm_Function.nm_Parent_ID = ( fn->fkt_Parent ) ? fn->fkt_Parent->fkt_NotifyID : hcdid;
			msg->nm_Data.nm_Function.nm_PortNr	  = fn->fkt_PortNr;

			usbbase->usb_IExec->AddTail ( snapshot, &msg->nm_Message.mn_Node );
		}

		n = n->un_Prev;
	}

	return ( TRUE );
}

// --

static S32
Send_Interfaces ( struct USBBase * usbbase, struct List * snapshot )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceGroup *  ig;
	struct USB2_InterfaceNode *	  in;
	struct USB2_NotifyMessage *	  msg;
	struct RealFunctionNode *	  fn;
	struct USB2_ConfigNode *	  cn;
	struct USB2_Node *			  n;

	fn = NULL;

	n = usbbase->usb_Fkt_Header.uh_Tail;

	while ( n )
	{
		fn = n->un_Data;

		if ( ( fn ) && ( fn->fkt_Initialized ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) )
		{
			cn = fn->fkt_Config_Active;

			if ( cn )
			{
				ig = cn->cfg_InterfaceGroups.uh_Head;

				while ( ig )
				{
					ih = ig->ig_Group.uh_Head;

					while ( ih )
					{
						in = ih->ih_Active;

						if ( ! in )
						{
							ih = Node_Next ( ih );
							continue;
						}

						msg = MEMORY_ALLOC ( MEMID_NotifyMessage, TRUE );

						if ( ! msg )
						{
							return ( FALSE );
						}

						// Default Notify Data

						msg->nm_Message.mn_ReplyPort = &usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
						msg->nm_Message.mn_Length	 = sizeof ( struct USB2_NotifyMessage );
						msg->nm_Type				 = USBNT_Interface;
						msg->nm_Command				 = USBNC_Adding;
						msg->nm_ID					 = ih->ih_NotifyID;

						// Function Specific Info

						msg->nm_Data.nm_Interface.nm_Config_Nr			= cn->cfg_Number;
						msg->nm_Data.nm_Interface.nm_Config_StringID	= cn->cfg_StringID;
						msg->nm_Data.nm_Interface.nm_Function_ID		= fn->fkt_NotifyID;
						msg->nm_Data.nm_Interface.nm_Interface_Nr		= ih->ih_Number;
						msg->nm_Data.nm_Interface.nm_Interface_AltNr	= ih->ih_AltNumber;
						msg->nm_Data.nm_Interface.nm_Interface_StringID = in->in_StringID;

						usbbase->usb_IExec->AddTail ( snapshot, &msg->nm_Message.mn_Node );

						ih = Node_Next ( ih );
					}

					ig = Node_Next ( ig );
				}
			}
		}

		n = n->un_Prev;
	}

	return ( TRUE );
}

// --
