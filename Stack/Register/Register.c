
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static struct USB2_InterfaceHeader *
LockPublishedInterface ( struct USBBase * usbbase, const struct USB2_Interface * ifc, const USB2_ID * id )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceHeader * found;
	struct USB2_InterfaceGroup *  ig;
	struct USB2_ConfigNode *	  config;
	struct RealFunctionNode *	  fn;
	struct USB2_Node *			  node;

	found = NULL;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	for ( node = usbbase->usb_Fkt_Header.uh_Head; ( node ) && ( ! found ); node = Node_Next ( node ) )
	{
		fn	   = node->un_Data;
		config = ( fn ) ? fn->fkt_Config_Active : NULL;

		if ( ( ! config ) || ( ! fn->fkt_Initialized ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) )
		{
			continue;
		}

		for ( ig = config->cfg_InterfaceGroups.uh_Head; ( ig ) && ( ! found ); ig = Node_Next ( ig ) )
		{
			for ( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next ( ih ) )
			{
				if ( ( ( &ih->ih_Public == ifc ) || ( ( id ) && ( ih->ih_NotifyID == *id ) ) )
					 && ( INTERFACE_LOCKHEADER ( ih ) == LSTAT_Okay ) )
				{
					found = ih;
					break;
				}
			}
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );

	return ( found );
}

// --

SEC_CODE static struct RealFunctionNode *
LockPublishedFunction ( struct USBBase * usbbase, const struct USB2_Function * function )
{
	struct USB2_Node *		  node;
	struct RealFunctionNode * fn;

	fn = NULL;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	for ( node = usbbase->usb_Fkt_Header.uh_Head; node; node = Node_Next ( node ) )
	{
		fn = node->un_Data;

		if ( ( fn ) && ( &fn->fkt_Public == function ) && ( fn->fkt_ControlEndPoint ) && ( ! fn->fkt_Detach )
			 && ( ! fn->fkt_FreeMe ) && ( FUNCTION_LOCK ( fn ) == LSTAT_Okay ) )
		{
			break;
		}

		fn = NULL;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );

	return ( fn );
}

// --

SEC_CODE struct RealRegister *
__Register_RegisterList ( struct USBBase * usbbase, struct TagItem * taglist )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceHeader * pinned;
	const struct USB2_Interface * ifc;
	const struct USB2_Function *  function;
	struct RealFunctionNode *	  pinned_fn;
	const USB2_ID *				  ifc_id;
	struct USB2_DriverMessage *	  msg;
	struct RealFunctionNode *	  fn;
	struct RealRegister *		  reg;
	struct TagItem *			  tag;
	U32							  timeout;
	S32							  error;
	STR							  title;
	// S32 dev;

#if 0
struct USB2_InterfaceGroup *ig;
struct USB2_ConfigNode *cn;
#endif

	//	USBERROR( "USB2 Stack : __Register_RegisterList" );

	//	timeout	= 10 * 1000000;	// 10 sec timeout
	timeout	  = 500000; // 0.5 sec timeout
	reg		  = NULL;
	msg		  = NULL;
	fn		  = NULL;
	ih		  = NULL;
	ifc		  = NULL;
	function  = NULL;
	pinned_fn = NULL;
	ifc_id	  = NULL;
	pinned	  = NULL;
	error	  = TRUE;
	//	dev		= FALSE;
	title = NULL;

	while ( ( tag = MISC_NEXTTAGITEM ( &taglist ) ) )
	{
		switch ( tag->ti_Tag )
		{
			case USB2Tag_Reg_DriverMessage:
			{
				msg		 = (PTR)tag->ti_Data;
				function = NULL;
				ifc		 = NULL;
				ifc_id	 = NULL;
				break;
			}

			case USB2Tag_Reg_Function:
			{
				msg		 = NULL;
				function = (PTR)tag->ti_Data;
				ifc		 = NULL;
				ifc_id	 = NULL;
				break;
			}

			case USB2Tag_Reg_Interface:
			{
				msg		 = NULL;
				function = NULL;
				ifc		 = (PTR)tag->ti_Data;
				ifc_id	 = NULL;
				break;
			}

			case USB2Tag_Reg_InterfaceID:
			{
				msg		 = NULL;
				function = NULL;
				ifc		 = NULL;
				ifc_id	 = (PTR)tag->ti_Data;
				break;
			}

			case USB2Tag_Reg_TimeOut:
			{
				USBINFO ( "USB2Tag_Reg_TimeOut .... : %ld ms", tag->ti_Data );

				// Zero is valid
				timeout = tag->ti_Data;
				break;
			}

			case USB2Tag_Reg_Title:
			{
				title = (STR)tag->ti_Data;
				USBINFO ( "USB2Tag_Reg_Title ...... : %s", ( title ) ? title : "<NULL>" );
				break;
			}

			default:
			{
				USBINFO ( "__Register_RegisterList  : Unknown tag [%08lx:%08lx]", tag->ti_Tag, tag->ti_Data );
				break;
			}
		}
	}

	if ( ( ifc ) || ( ifc_id ) )
	{
		pinned = LockPublishedInterface ( usbbase, ifc, ifc_id );

		ih = pinned;
	}

	if ( function )
	{
		pinned_fn = LockPublishedFunction ( usbbase, function );
		fn		  = pinned_fn;
	}

	if ( ( ! msg ) && ( ! fn ) && ( ! ih ) )
	{
		USBERROR ( "__Register_RegisterList : Tag Missing" );
		goto bailout;
	}

	//	USBERROR( "Register [ MSG:%p, FN:%p, IH:%p ]", msg, fn, ih );

	reg = REGISTER_ALLOC ();

	if ( ! reg )
	{
		USBERROR ( "__Register_RegisterList  : Error allocating memory" );
		goto bailout;
	}

	reg->reg_Title = ( title ) ? title : "";

	// ----
	// -- Figure out how to 'Take over' from Message

	if ( msg )
	{
		if ( msg->Interface )
		{
			//			ih = (PTR) msg->Interface;
			ih = (PTR)( (char *)msg->Interface - offsetof ( struct USB2_InterfaceHeader, ih_Public ) );

			if ( INTERFACE_VALIDHEADER ( ih ) != VSTAT_Okay )
			{
				USBERROR ( "__Register_RegisterList  : Error Invalid Interface" );
				goto bailout;
			}
		}
		else
		{
			fn = (PTR)msg->Function;

			if ( FUNCTION_VALID ( fn ) != VSTAT_Okay )
			{
				USBERROR ( "__Register_RegisterList  : Error Invalid Function" );
				goto bailout;
			}
		}
	}

	// -- Take over Device

#if 0
	if ( dev )
	{
		// We need to take full controll over all the resources
//		USBDEBUG( "Register_RegisterList: Take over Device" );
		USBDEBUG( "Take over Device" );

		fn = (PTR) msg->Function;

		if ( FUNCTION_LOCK( fn ) != LSTAT_Okay )
		{
			USBDEBUG( 1, "Register_RegisterList: Claim Failed" );
			goto bailout;
		}

		IExec->DebugPrintF( "_L_#512.1 : %p : reg_Function\n", fn );
		reg->rr_Function = fn;

		if ( ! FUNCTION_CLAIM( reg, fn ))
		{
			USBDEBUG( 1, "Register_RegisterList: Claim Failed" );
			goto bailout;
		}

		// -- Claim all, but only Active Cfg ?

		cn = fn->fkt_ConfigActive;
		ig = cn->cfg_InterfaceGroups.uh_Head;
		ih = ig->ig_Group.uh_Head;

		while( ih )
		{
			// hmm claim group?? instead of number
			if ( ! INTERFACE_CLAIMHEADER( reg, ih->ih_Number ))
			{
				USBDEBUG( 1, "Register_RegisterList: Claim Failed" );
				goto bailout;
			}
			else
			{
				ih = Node_Next( ih );
			}
		}
	}

	// -- Take over Function

	else
#endif
	if ( fn )
	{
		//		USBERROR( "Take over Function" );

		if ( FUNCTION_LOCK ( fn ) != LSTAT_Okay )
		{
			USBERROR ( "__Register_RegisterList : Function Lock Error" );
			goto bailout;
		}

		reg->reg_Function = fn;

		// Claim also checks Detach
		if ( ! FUNCTION_CLAIM ( reg, fn ) )
		{
			USBERROR ( "__Register_RegisterList : Claim Failed 2" );
			goto bailout;
		}
	}

	// -- Take over Interface

	else if ( ih )
	{
		//		USBERROR( "Take over Interface" );

		if ( INTERFACE_LOCKHEADER ( ih ) != LSTAT_Okay )
		{
			USBERROR ( "__Register_RegisterList : Claim Failed 3" );
			goto bailout;
		}

		reg->reg_Interface = ih;

		fn = ih->ih_Function;

		if ( FUNCTION_LOCK ( fn ) != LSTAT_Okay )
		{
			USBERROR ( "__Register_RegisterList : Claim Failed 4" );
			goto bailout;
		}

		reg->reg_Function = fn;

		if ( ! INTERFACE_CLAIMHEADER ( reg, ih ) )
		{
			USBERROR ( "__Register_RegisterList : Claim Failed 5" );
			goto bailout;
		}
	}

	// -- Take : Fail

	else
	{
		USBERROR ( "__Register_RegisterList  : Error" );
		goto bailout;
	}

	// --

	reg->reg_Public.Res_Control = (PTR)ENDPOINTRES_OBTAINTAGS ( reg, USB2Tag_EPRes_TimeOut, timeout, TAG_END );

	if ( ! reg->reg_Public.Res_Control )
	{
		USBERROR ( "__Register_RegisterList  : Error creating Control EndPoint" );
		goto bailout;
	}

	reg->reg_Public.Stack_MsgPort	 = &reg->reg_MsgPort.ump_MsgPort;
	reg->reg_Public.Stack_MsgPortBit = reg->reg_MsgPort.ump_Signal.sig_Signal_Mask;
	reg->reg_Public.HCD_ID			 = reg->reg_Function->fkt_HCD->hn_NotifyID;
	reg->reg_Public.FKT_ID			 = reg->reg_Function->fkt_NotifyID;

	// --

	error = FALSE;

bailout:

	if ( ( reg ) && ( error ) )
	{
		REGISTER_UNREGISTER ( reg );
		reg = NULL;
	}

	if ( pinned )
	{
		INTERFACE_UNLOCKHEADER ( pinned );
	}

	if ( pinned_fn )
	{
		FUNCTION_UNLOCK ( pinned_fn );
	}

	//	USBDEBUG( "Register_RegisterList: 99" );

	return ( reg );
}

// --

SEC_CODE struct RealRegister * VARARGS68K
__Register_RegisterTags ( struct USBBase * usbbase, ... )
{
	struct RealRegister * reg;
	va_list				  ap;

	va_start ( ap, usbbase );

	reg = REGISTER_REGISTERLIST ( va_getlinearva ( ap, struct TagItem * ) );

	va_end ( ap );

	return ( reg );
}

// --
