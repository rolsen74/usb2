
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
** When we mark a Function as Detached,
** it means the stack should not use it anymore
** and Resources should be free'd.
**
** Either the Device have been detached or
** the owner has been detached.
**
** We do not free anything here, only mark
*/

// --

/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if 0

struct CmdHeader
{
	struct Message		cm_Message;
	enum USB2_Cmd		cm_Command;
	struct USBBase *	cm_usbbase;
	U32					cm_Result;
	struct Task *		cm_Task;
	U32					cm_Mask;
};

#endif

static void
__Send_DetachCommand ( struct USBBase * usbbase, struct RealFunctionNode * fn )
{
	struct FktMessage * fm;

	fm = MEM_ALLOCVEC ( sizeof ( struct FktMessage ), TRUE );

	if ( ! fm )
	{
		USBERROR ( "Error allocating memory fm" );
		goto bailout;
	}

	fm->fm_Cmd.cm_Command = CMD_FKT_DETACH;

	// --

	if ( FUNCTION_LOCK ( fn ) != LSTAT_Okay )
	{
		MEM_FREEVEC ( fm );
		goto bailout;
	}

	fm->fm_FN = fn;

	// --

	MSGPORT_PUTMSG ( usbbase->usb_Master_MsgPort, fm );

bailout:

	return;
}

// --

SEC_CODE static void
NotifyOwner ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct RealRegister * reg )
{
	struct USB2_NotifyMessage * msg;

	if ( ( ! reg ) || ( reg->reg_FreeMe ) || ( reg->reg_Detach ) || ( ! usbbase->usb_Notify_ReplyMsgPort ) )
	{
		return;
	}

	msg = MEMORY_ALLOC ( MEMID_NotifyMessage, TRUE );

	if ( ! msg )
	{
		return;
	}

	reg->reg_Detach					   = TRUE;
	msg->nm_Message.mn_ReplyPort	   = &usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
	msg->nm_Message.mn_Length		   = sizeof ( *msg );
	msg->nm_Type					   = USBNT_Function;
	msg->nm_Command					   = USBNC_Detached;
	msg->nm_ID						   = fn->fkt_NotifyID;
	msg->nm_Data.nm_Function.nm_HCD_ID = fn->fkt_Public.HCD_ID;

	MSGPORT_PUTMSG ( &reg->reg_MsgPort, msg );
}

// --

SEC_CODE void
__Function_MarkDetach ( struct USBBase * usbbase, struct RealFunctionNode * fn )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceGroup *  ig;
	struct USB2_InterfaceNode *	  in;
	struct USB2_EndPointNode *	  ep;
	struct USB2_ConfigNode *	  cn;

	TASK_NAME_ENTER ( "__Function_MarkDetach" );

	USBDEBUG ( "__Function_MarkDetach    : FN    %p", fn );

	if ( FUNCTION_LOCK ( fn ) == LSTAT_Okay )
	{
		SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );
		if ( ! fn->fkt_Detach )
		{
			USBINFO ( "Marking FN %p Detached", fn );
			__Send_DetachCommand ( usbbase, fn );
			fn->fkt_Detach = TRUE;

			NotifyOwner ( usbbase, fn, fn->fkt_Owner );

			if ( fn->fkt_Initialized )
			{
				USB2_Notify_Function ( usbbase, fn, USBNC_Removing );
			}
		}

		cn = fn->fkt_Configs.uh_Head;

		while ( cn )
		{
			if ( ! cn->cfg_Detach )
			{
				USBINFO ( "Marking CN %p Detached", cn );
				cn->cfg_Detach = TRUE;
			}

			ig = cn->cfg_InterfaceGroups.uh_Head;

			while ( ig )
			{
				if ( ! ig->ig_Detach )
				{
					USBINFO ( "Marking IG %p Detached", ig );
					ig->ig_Detach = TRUE;
				}

				ih = ig->ig_Group.uh_Head;

				while ( ih )
				{
					if ( ! ih->ih_Detach )
					{
						USBINFO ( "Marking IH %p Detached", ih );
						ih->ih_Detach = TRUE;

						NotifyOwner ( usbbase, fn, ih->ih_Owner );
					}

					in = ih->ih_AltSettings.uh_Head;

					while ( in )
					{
						if ( ! in->in_Detach )
						{
							USBINFO ( "Marking IN %p Detached", in );
							in->in_Detach = TRUE;
						}

						ep = in->in_EndPoints.uh_Head;

						while ( ep )
						{
							if ( ! ep->ep_Detach )
							{
								USBINFO ( "Marking EP %p Detached", ep );
								ep->ep_Detach = TRUE;
							}

							// --
							// I nolonger abort IORequest here
							// --

							ep = Node_Next ( ep );
						}

						in = Node_Next ( in );
					}

					ih = Node_Next ( ih );
				}

				ig = Node_Next ( ig );
			}

			cn = Node_Next ( cn );
		}

		if ( ( fn->fkt_HCD ) && ( fn->fkt_HCD->hn_Task ) && ( fn->fkt_HCD->hn_Task->tn_TaskAdr ) )
		{
			fn->fkt_HCD->hn_DetachPending = TRUE;

			TASK_SIGNAL ( fn->fkt_HCD->hn_Task->tn_TaskAdr, fn->fkt_HCD->hn_HUB_Signal.sig_Signal_Mask );
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

		FUNCTION_UNLOCK ( fn );
	}

	TASK_NAME_LEAVE ();
}

// --
