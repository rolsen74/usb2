
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static U32
_Start_Function ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_FktDriverNode * fdn,
				  struct USB2_ASync * as, const struct DriverSearch * search )
{
	struct USB2_DriverNode * dn;
	struct USB2_TaskNode *	 tn;
	enum TaskReturn			 stat;
	struct USB2_Signal		 ready_signal;
	S32						 isRootHUB;
	U32						 retval;

	TASK_NAME_ENTER ( "_Start_Function" );

	retval = TASK_Return_Stack_Error;
	tn	   = NULL;

	if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) )
	{
		goto bailout;
	}

	dn = AllocDriver ( usbbase, fn, fdn, as, search );

	if ( ! dn )
	{
		USBERROR ( "_Start_Function          : Error allocating memory" );
		goto bailout;
	}

	// --

	if ( ( fn->fkt_Tier == 0 ) // RootHub
		 && ( fn->fkt_Class == USBCLASS_HUB ) )
	{
		isRootHUB = TRUE;

		USBDEBUG ( "_Start_Function : is RootHUB" );
		if ( ! TASK_ALLOCSIGNAL ( &ready_signal ) )
		{
			DiscardDriver ( usbbase, dn );

			goto bailout;
		}

		dn->dn_Message.rdm_Mask	  = ready_signal.sig_Signal_Mask;
		dn->dn_Message.rdm_Parent = TASK_FIND ();
	}
	else
	{
		isRootHUB = FALSE;

		USBDEBUG ( "_Start_Function : is NOT RootHUB" );
		dn->dn_Message.rdm_Parent = NULL;
	}

	// -- Start Driver

	stat = TASK_START ( TASK_Tag_Func_Init, DriverWorkerInit, TASK_Tag_Func_Main, DriverWorkerMain, TASK_Tag_Func_Free,
						DriverWorkerFree, TASK_Tag_UserData, dn, TASK_Tag_Get_TN, &tn, TASK_Tag_Prioity, fdn->fdn_Pri,
						TASK_Tag_ASync, dn->dn_ASync, TASK_Tag_Type, TASK_Type_FKT, TASK_Tag_Name, fdn->fdn_Title, TAG_END );

	// --

	if ( isRootHUB )
	{
		if ( ( ! myIS_TASKRETURN_ERR ( stat ) ) && ( tn ) )
		{
			TASK_WAIT ( ready_signal.sig_Signal_Mask );

			stat = dn->dn_Message.rdm_Result;
		}

		TASK_FREESIGNAL ( &ready_signal );
	}

	retval = stat;

	if ( ( ! myIS_TASKRETURN_ERR ( stat ) ) && ( stat != TASK_Return_Unsupported ) && ( tn ) )
	{
		if ( ! PublishDriver ( usbbase, dn, tn ) )
		{
			retval = TASK_Return_Error;
		}
	}
	else
	{
		if ( tn )
		{
			TASK_UNLOCK ( tn );
		}

		if ( ( ! myIS_TASKRETURN_ERR ( stat ) ) && ( stat != TASK_Return_Unsupported ) )
		{
			retval = TASK_Return_Stack_Error;
		}

		DiscardDriver ( usbbase, dn );
	}

bailout:

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --

SEC_CODE static enum FDSTAT
_Check_for_Functions ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as,
					   struct DriverSearch * search )
{
	struct USB2_FktDriverNode * fdn;

	U32 taskstat;

	USBDEBUG ( "fkt ............ : %p", fn );
	USBDEBUG ( "fkt_Address .... : %ld", fn->fkt_Address );
	USBDEBUG ( "fkt_Tier ....... : %ld", fn->fkt_Tier );
	USBDEBUG ( "fkt_Class ...... : %ld", fn->fkt_Class );
	USBDEBUG ( "fkt_SubClass ... : %ld", fn->fkt_SubClass );
	USBDEBUG ( "fkt_Protocol ... : %ld", fn->fkt_Protocol );

	if ( fn->fkt_Owner )
	{
		return ( FDSTAT_Not_Found );
	}

	for ( fdn = Header_Head ( &usbbase->usb_FDriver_Header ); fdn; fdn = Node_Next ( fdn ) )
	{
		if ( ( fdn->fdn_Type != USB2Val_FDriver_Function ) || ( ! TryDriverClass ( search, fdn ) ) )
		{
			continue;
		}

		if ( ! Match_Driver_Device ( usbbase, fdn, fn->fkt_DeviceDescriptor ) )
		{
			continue;
		}

		if ( ( ( fdn->fdn_Class != fn->fkt_Class ) && ( fdn->fdn_Class != FDCLASS_Any ) )
			 || ( ( fdn->fdn_SubClass != fn->fkt_SubClass ) && ( fdn->fdn_SubClass != FDSUBCLASS_Any ) )
			 || ( ( fdn->fdn_Protocol != fn->fkt_Protocol ) && ( fdn->fdn_Protocol != FDPROTOCOL_Any ) ) )
		{
			USBDEBUG ( "FKT : class %ld/%ld/%ld does not match %ld/%ld/%ld",
					   fn->fkt_Class, fn->fkt_SubClass, fn->fkt_Protocol, fdn->fdn_Class, fdn->fdn_SubClass, fdn->fdn_Protocol );
			continue;
		}

		taskstat = _Start_Function ( usbbase, fn, fdn, as, search );

		if ( ( taskstat == TASK_Return_Okay ) || ( taskstat == TASK_Return_Stack_Okay ) )
		{
			return ( FDSTAT_Found );
		}

		if ( ( taskstat == TASK_Return_Error ) || ( taskstat == TASK_Return_Stack_Error ) )
		{
			if ( ! fn->fkt_Tier )
			{
				USBDEBUG ( "Function error (Stat $%08lx)", taskstat );

				return ( FDSTAT_Error );
			}

			USBDEBUG ( "Failed to start Function" );
		}
	}

	return ( FDSTAT_Not_Found );
}

// --
