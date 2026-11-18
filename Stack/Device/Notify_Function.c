/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE void
USB2_Notify_Function( struct USBBase *usbbase, struct RealFunctionNode *fn, U32 command )
{
	struct USB2_NotifyNode *node;
	struct USB2_NotifyMessage *msg;
	struct USB2_ConfigNode *cn;
	struct USB2_InterfaceGroup *ig;
	struct USB2_InterfaceHeader *ih;
	struct USB2_InterfaceNode *in;

	if ( ( ! fn ) || ( ! fn->fkt_HCD ) || ( ! usbbase->usb_Notify_ReplyMsgPort ) )
	{
		return;
	}

	SEMAPHORE_OBTAIN( & usbbase->usb_Notify_Semaphore );

	for ( node = usbbase->usb_Notify_Header.uh_Head; node; node = Node_Next( node ) )
	{
		if ( node->nn_Type != USBNT_Function )
		{
			continue;
		}

		msg = MEMORY_ALLOC( MEMID_NotifyMessage, TRUE );

		if ( ! msg )
		{
			continue;
		}

		msg->nm_Message.mn_ReplyPort = & usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
		msg->nm_Message.mn_Length = sizeof( *msg );
		msg->nm_Type = USBNT_Function;
		msg->nm_Command = command;
		msg->nm_ID = fn->fkt_NotifyID;
		msg->nm_Data.nm_Function.nm_HCD_ID = fn->fkt_HCD->hn_NotifyID;
		msg->nm_Data.nm_Function.nm_Parent_ID = fn->fkt_Parent ?
			fn->fkt_Parent->fkt_NotifyID : fn->fkt_HCD->hn_NotifyID;
		msg->nm_Data.nm_Function.nm_PortNr = fn->fkt_PortNr;

		usbbase->usb_IExec->PutMsg( node->nn_MsgPort, & msg->nm_Message );
	}

	if ( command == USBNC_Adding )
	{
		cn = fn->fkt_Config_Active;

		if ( cn )
		{
			for ( node = usbbase->usb_Notify_Header.uh_Head; node; node = Node_Next( node ) )
			{
				if ( node->nn_Type != USBNT_Interface )
				{
					continue;
				}

				for ( ig = cn->cfg_InterfaceGroups.uh_Head; ig; ig = Node_Next( ig ) )
				{
					for ( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next( ih ) )
					{
						in = ih->ih_Active;

						if ( ! in )
						{
							continue;
						}

						msg = MEMORY_ALLOC( MEMID_NotifyMessage, TRUE );

						if ( ! msg )
						{
							continue;
						}

						msg->nm_Message.mn_ReplyPort = & usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
						msg->nm_Message.mn_Length = sizeof( *msg );
						msg->nm_Type = USBNT_Interface;
						msg->nm_Command = USBNC_Adding;
						msg->nm_ID = ih->ih_NotifyID;
						msg->nm_Data.nm_Interface.nm_Config_Nr = cn->cfg_Number;
						msg->nm_Data.nm_Interface.nm_Config_StringID = cn->cfg_StringID;
						msg->nm_Data.nm_Interface.nm_Function_ID = fn->fkt_NotifyID;
						msg->nm_Data.nm_Interface.nm_Interface_Nr = ih->ih_Number;
						msg->nm_Data.nm_Interface.nm_Interface_AltNr = ih->ih_AltNumber;
						msg->nm_Data.nm_Interface.nm_Interface_StringID = in->in_StringID;

						usbbase->usb_IExec->PutMsg( node->nn_MsgPort, & msg->nm_Message );
					}
				}
			}
		}
	}

	SEMAPHORE_RELEASE( & usbbase->usb_Notify_Semaphore );
}

// --
