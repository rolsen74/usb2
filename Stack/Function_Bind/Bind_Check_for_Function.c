
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
				  struct USB2_ASync * as )
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

	dn = DRIVER_ALLOC ( fn, as );

	if ( ! dn )
	{
		USBERROR ( "_Start_Function          : Error allocating memory" );
		goto bailout;
	}

	dn->dn_Class	= fdn;
	dn->dn_Entry	= fdn->fdn_Entry;
	dn->dn_Filename = fdn->fdn_Driver_Filename;

	// -- Init Public Message

	// Alloc check FN
	//	if ( FUNCTION_VALID(fn) != VSTAT_Okay )
	//	{
	//		USBDEBUG( "_Start_Function : Function Valid error (%p)", fn );
	//		goto bailout;
	//	}

	dn->dn_Message.rdm_Public.IUSB2				= usbbase->usb_IUSB2;
	dn->dn_Message.rdm_Public.VendorID			= LE_SWAP16 ( fn->fkt_DeviceDescriptor->VendorID );
	dn->dn_Message.rdm_Public.DeviceID			= LE_SWAP16 ( fn->fkt_DeviceDescriptor->DeviceID );
	dn->dn_Message.rdm_Public.Function			= (PTR)fn;
	dn->dn_Message.rdm_Public.ConfigDescriptors = (PTR)fn->fkt_Config_Desc_Buf;
	dn->dn_Message.rdm_Public.ClassTitle		= fdn->fdn_Title;
	dn->dn_Message.rdm_Public.ClassDriver		= fdn->fdn_Driver_Filename;
	dn->dn_Message.rdm_Public.ClassAlias		= fdn->fdn_Alias;

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
						TASK_Tag_ASync, as, TASK_Tag_Type, TASK_Type_FKT, TASK_Tag_Name, fdn->fdn_Title, TAG_END );

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

static enum FDSTAT
_Check_for_Functions ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as,
							struct USB2_FktDriverNode * previous )
{
	struct USB2_FktDriverNode * fdn;
	enum FDSTAT					stat;
	U32							taskstat;

	//	USBERROR( "_Check_for_Functions" );
	USBDEBUG ( "fkt ............ : %p", fn );
	USBDEBUG ( "fkt_Address .... : %ld", fn->fkt_Address );
	USBDEBUG ( "fkt_Tier ....... : %ld", fn->fkt_Tier );
	USBDEBUG ( "fkt_Class ...... : %ld", fn->fkt_Class );
	USBDEBUG ( "fkt_SubClass ... : %ld", fn->fkt_SubClass );
	USBDEBUG ( "fkt_Protocol ... : %ld", fn->fkt_Protocol );

	stat = FDSTAT_Not_Found;

	if ( ! fn->fkt_Owner )
	{
		fdn = previous ? Node_Next ( previous ) : Header_Head ( &usbbase->usb_FDriver_Header );

		while ( fdn )
		{
			while ( fdn->fdn_Type == USB2Val_FDriver_Function )
			{
				//				usbbase->usb_IExec->DebugPrintF( "FDN Check Function : %p\n", fdn );

				if ( ! ClassAvailable ( usbbase, fdn ) )
				{
					//					USBERROR( "FKT : fdn_Detach" );
					break;
				}

				if ( ! Match_Driver_Device ( usbbase, fdn, fn->fkt_DeviceDescriptor ) )
				{
					break;
				}

				if ( ( fdn->fdn_Class != fn->fkt_Class ) && ( fdn->fdn_Class != FDCLASS_Any ) )
				{
					//					USBERROR( "FKT : fdn_Class : %ld : %ld :", fdn->fdn_Class, fn->fkt_Class );
					break;
				}

				if ( ( fdn->fdn_SubClass != fn->fkt_SubClass ) && ( fdn->fdn_SubClass != FDSUBCLASS_Any ) )
				{
					//					USBERROR( "FKT : fdn_SubClass : %ld : %ld :", fdn->fdn_SubClass, fn->fkt_SubClass );
					break;
				}

				if ( ( fdn->fdn_Protocol != fn->fkt_Protocol ) && ( fdn->fdn_Protocol != FDPROTOCOL_Any ) )
				{
					//					USBERROR( "FKT : fdn_Protocol : %ld : %ld :", fdn->fdn_Protocol, fn->fkt_Protocol );
					break;
				}

				//				USBDEBUG( "Found Function [ possible : '%s' ]", (fdn->fdn_Title)?fdn->fdn_Title:"" );
				//				USBERROR( "Found Function [ possible : '%s' ]", (fdn->fdn_Title)?fdn->fdn_Title:"" );

				taskstat = _Start_Function ( usbbase, fn, fdn, as );

				//				USBDEBUG( "_Start_Function return (Stat $%08lx)", taskstat );

				if ( ( taskstat == TASK_Return_Okay ) || ( taskstat == TASK_Return_Stack_Okay ) )
				{
					//					USBDEBUG( "Found Function" );
					//					USBERROR( "Found Function" );
					stat = FDSTAT_Found;
					break;
				}
				else if ( ( taskstat == TASK_Return_Error ) || ( taskstat == TASK_Return_Stack_Error ) )
				{
					if ( fn->fkt_Tier == 0 ) // Tier 0 = RootHUB
					{
						USBDEBUG ( "Function error (Stat $%08lx)", taskstat );
						stat = FDSTAT_Error;
						goto bailout;
					}
					else
					{
						// maybe do a Workbench notify in the future
						USBDEBUG ( "Failed to start Function" );
					}
				}

				stat = FDSTAT_Not_Found;
				break;
			}

			if ( stat == FDSTAT_Found )
			{
				break;
			}

			fdn = Node_Next ( fdn );
		}
	}

bailout:

	return ( stat );
}

// --
