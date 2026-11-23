
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --
// Release all resource we can, for this Struct

// The caller holds the stack lock.
SEC_CODE static S32
OwnsReplyPorts ( struct USBBase * usbbase, struct RealRegister * reg )
{
	struct RealEndPointResource * epr;
	struct Task * task;

	task = TASK_FIND ();

	if ( ( reg->reg_MsgPort.ump_StructID == ID_USB2_MP ) && ( reg->reg_MsgPort.ump_MsgPort.mp_SigTask != task ) )
	{
		return ( FALSE );
	}

	for ( epr = reg->reg_EPRHeader.uh_Head; epr; epr = Node_Next ( epr ) )
	{
		if ( ( epr->epr_ReplyOwner ) && ( epr->epr_ReplyOwner != task ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

SEC_CODE static void
__Release ( struct USBBase * usbbase, struct RealRegister * reg )
{
	struct RealEndPointResource * epr;
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceGroup *  ig;
	struct RealFunctionNode *	  fn;
	struct USB2_ConfigNode *	  cn;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	reg->reg_Public.Res_Control = NULL;

	while ( ( epr = reg->reg_EPRHeader.uh_Head ) )
	{
		epr->epr_FreeMe = TRUE;

		NODE_REMNODE ( &reg->reg_EPRHeader, &epr->epr_Node );

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		ENDPOINTRES_FREE ( epr );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );
	}

	// Other references include a private Obtain and an explicit Release.
	while ( reg->reg_Locks )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		MISC_WAIT ( 1000 );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );
	}

	// Only the teardown task can take the references used by the declaim helpers.
	reg->reg_ClosingTask = TASK_FIND ();

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	// -- Release Claims and Function Lock

	fn = reg->reg_Function;

	if ( ( fn ) && ( FUNCTION_VALID ( fn ) == VSTAT_Okay ) )
	{
		SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		// A detached configuration can still contain claimed interfaces.
		for ( cn = fn->fkt_Configs.uh_Head; cn; cn = Node_Next ( cn ) )
		{
			for ( ig = cn->cfg_InterfaceGroups.uh_Head; ig; ig = Node_Next ( ig ) )
			{
				for ( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next ( ih ) )
				{
					if ( ih->ih_Owner == reg )
					{
						INTERFACE_DECLAIMHEADER ( reg, ih );
					}
				}
			}
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

		if ( fn->fkt_Owner == reg )
		{
			FUNCTION_DECLAIM ( reg, fn );
		}

		FUNCTION_UNLOCK ( fn );
		reg->reg_Function = NULL;
	}

	// --

	ih = reg->reg_Interface;

	if ( ( ih ) && ( INTERFACE_VALIDHEADER ( ih ) == VSTAT_Okay ) )
	{
		INTERFACE_UNLOCKHEADER ( ih );
		reg->reg_Interface = NULL;
	}
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE enum FSTAT
__Register_Free ( struct USBBase * usbbase, struct RealRegister * reg, STR file UNUSED )

#else

SEC_CODE enum FSTAT
__Register_Free ( struct USBBase * usbbase, struct RealRegister * reg )

#endif

{
	struct Message * msg;

	enum VSTAT vstat;
	enum FSTAT fstat;

	TASK_NAME_ENTER ( "__Register_Free" );

	USBERROR ( "__Register_Free          : REG   %p : (%s)", reg, ( file ) ? file : "<NULL>" );

	// --

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	vstat = REGISTER_VALID ( reg );

	if ( vstat != VSTAT_Okay )
	{
		fstat = ( vstat == VSTAT_Null ) ? FSTAT_Null : FSTAT_Error;
	}
	else if ( reg->reg_FreeMe )
	{
		fstat = FSTAT_Locked;
	}
	else if ( ! OwnsReplyPorts ( usbbase, reg ) )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB2: unregister on the task owning the registration and reply ports\n" );

		fstat = FSTAT_Error;
	}
	else
	{
		reg->reg_FreeMe = TRUE;

		if ( reg->reg_FunctionNode.un_Data )
		{
			NODE_REMNODE ( &reg->reg_Function->fkt_Registrations, &reg->reg_FunctionNode );

			reg->reg_FunctionNode.un_Data = NULL;
		}

		fstat = FSTAT_Okay;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( fstat != FSTAT_Okay )
	{
		goto bailout;
	}

	__Release ( usbbase, reg );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	reg->reg_StructID = ID_USB2_FREED;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	USBINFO ( "__Register_Free          : Freeing %p : (%s)", reg, ( file ) ? file : "<NULL>" );

	// --

	//	#ifdef DO_PANIC
	//
	//  We do not have a Node in front
	//	if ((( Node_Next( reg )) && ( Node_Next( reg ) != (PTR) 0xAC111111 ))
	//	||	(( Node_Prev( reg )) && ( Node_Prev( reg ) != (PTR) 0xAC222222 )))
	//	{
	//		USBPANIC( "__Register_Free          : REGister Node have not been removed : Self %p : Next %p : Prev %p : (%s)", reg,
	// Node_Next( reg ), Node_Prev( reg ), file );
	//	}
	//
	//	#endif

	// --

	if ( reg->reg_MsgPort.ump_StructID == ID_USB2_MP )
	{
		while ( ( msg = MSGPORT_GETMSG ( &reg->reg_MsgPort ) ) )
		{
			MSGPORT_REPLYMSG ( msg );
		}

		MSGPORT_FREE ( &reg->reg_MsgPort );
	}

	if ( reg->reg_DetachMessage )
	{
		MEMORY_FREE ( MEMID_NotifyMessage, reg->reg_DetachMessage, 0 );
	}

	MEMORY_FREE ( MEMID_USBRegister, reg, 0 );

bailout:

	TASK_NAME_LEAVE ();

	return ( fstat );
}

// --
