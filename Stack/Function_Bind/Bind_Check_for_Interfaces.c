
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
_Start_Interface ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_FktDriverNode * fdn,
				   struct USB2_InterfaceHeader * ih, struct USB2_Interface_Desc * ifcdsc, struct USB2_ASync * as )
{
	struct USB2_DriverNode * dn;
	struct USB2_TaskNode *	 tn;
	enum TaskReturn			 stat;
	U32						 retval;

	TASK_NAME_ENTER ( "_Start_Interface" );

	USBDEBUG ( "_Start_Interface" );

	retval = TASK_Return_Stack_Error;
	tn	   = NULL;

	if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( ih->ih_Owner ) )
	{
		goto bailout;
	}

	dn = DRIVER_ALLOC ( fn, as );

	if ( ! dn )
	{
		USBERROR ( "_Start_Interface : Error allocating memory" );
		goto bailout;
	}

	dn->dn_ASync	= as;
	dn->dn_Class	= fdn;
	dn->dn_Entry	= fdn->fdn_Entry;
	dn->dn_Filename = fdn->fdn_Driver_Filename;

	// -- Init Public Message

	// Alloc check FN
	//	if ( FUNCTION_VALID(fn) != VSTAT_Okay )
	//	{
	//		USBERROR( "_Start_Interface : Function Valid error (%p)", fn );
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

	if ( INTERFACE_LOCKHEADER ( ih ) != LSTAT_Okay )
	{
		DRIVER_FREE ( dn );

		goto bailout;
	}

	dn->dn_Interface							  = ih;
	dn->dn_Message.rdm_Public.Interface			  = (PTR)&ih->ih_Public;
	dn->dn_Message.rdm_Public.InterfaceDescriptor = ifcdsc;

	// -- Start Driver

	stat = TASK_START ( TASK_Tag_Func_Init, DriverWorkerInit, TASK_Tag_Func_Main, DriverWorkerMain, TASK_Tag_Func_Free,
						DriverWorkerFree, TASK_Tag_UserData, dn, TASK_Tag_Get_TN, &tn, TASK_Tag_Prioity, fdn->fdn_Pri,
						TASK_Tag_ASync, as, TASK_Tag_Type, TASK_Type_FKT, TASK_Tag_Name, fdn->fdn_Title, TAG_END );

	// --

	retval = stat;

	if ( ( ! myIS_TASKRETURN_ERR ( stat ) ) && ( tn ) )
	{
		if ( ! PublishDriver ( usbbase, dn, tn ) )
		{
			retval = TASK_Return_Error;
		}
	}
	else
	{
		if ( ! myIS_TASKRETURN_ERR ( stat ) )
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

SEC_CODE static S32
_Check_Interface ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_InterfaceHeader * ih,
				   struct USB2_ASync * as, struct USB2_DriverNode * previous )
{
	struct USB2_FktDriverNode *	 fdn;
	struct USB2_DriverNode *	 bound;
	struct USB2_Interface_Desc * ifcdsc;
	enum DriverDeviceMatch		 wanted;

	S32 stat;
	S32 skip;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Bind_Semaphore );

	for ( bound = usbbase->usb_Bind_Header.uh_Head; bound; bound = Node_Next ( bound ) )
	{
		if ( ( bound != previous ) && ( bound->dn_Interface == ih ) && ( ! bound->dn_Detach ) && ( ! bound->dn_FreeMe ) )
		{
			break;
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_Bind_Semaphore );

	if ( bound )
	{
		return ( TRUE );
	}

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	ifcdsc = ( ( ih->ih_Active ) && ( ! ih->ih_Owner ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) )
				 ? ih->ih_Active->in_Descriptor
				 : NULL;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( ! ifcdsc )
	{
		return ( FALSE );
	}

	skip = previous != NULL;

	// Try device-specific classes first, keeping the existing order within each pass.
	for ( wanted = DriverMatch_ID; wanted != DriverMatch_None; wanted-- )
	{
		for ( fdn = Header_Head ( &usbbase->usb_FDriver_Header ); fdn; fdn = Node_Next ( fdn ) )
		{
			if ( ( fdn->fdn_Type != USB2Val_FDriver_Interface ) || ( ! ClassAvailable ( usbbase, fdn ) ) )
			{
				continue;
			}

			if ( Match_Driver_Device ( usbbase, fdn, fn->fkt_DeviceDescriptor ) != wanted )
			{
				continue;
			}

			if ( skip )
			{
				if ( fdn == previous->dn_Class )
				{
					skip = FALSE;
				}

				continue;
			}

#ifdef USB2_BIND_DEBUG
			if ( ( wanted == DriverMatch_ID ) && ( fdn->fdn_Driver_Filename ) )
			{
				usbbase->usb_IExec->DebugPrintF (
					"USB2 bind: '%s' ID match, interface %lu %02lX/%02lX/%02lX, filter %lX/%lX/%lX\n", fdn->fdn_Driver_Filename,
					(U32)ifcdsc->InterfaceNumber, (U32)ifcdsc->InterfaceClass, (U32)ifcdsc->InterfaceSubClass,
					(U32)ifcdsc->InterfaceProtocol, (U32)fdn->fdn_Class, (U32)fdn->fdn_SubClass, (U32)fdn->fdn_Protocol );
			}
#endif

			if ( ( ( fdn->fdn_Class != ifcdsc->InterfaceClass ) && ( fdn->fdn_Class != FDCLASS_Any ) )
				 || ( ( fdn->fdn_SubClass != ifcdsc->InterfaceSubClass ) && ( fdn->fdn_SubClass != FDSUBCLASS_Any ) )
				 || ( ( fdn->fdn_Protocol != ifcdsc->InterfaceProtocol ) && ( fdn->fdn_Protocol != FDPROTOCOL_Any ) ) )
			{
				continue;
			}

			stat = _Start_Interface ( usbbase, fn, fdn, ih, ifcdsc, as );

#ifdef USB2_BIND_DEBUG
			if ( fdn->fdn_Driver_Filename )
			{
				usbbase->usb_IExec->DebugPrintF ( "USB2 bind: '%s' interface %lu start result $%08lX\n",
												  fdn->fdn_Driver_Filename, (U32)ifcdsc->InterfaceNumber, (U32)stat );
			}
			else if ( ( fdn->fdn_Entry ) && ( fdn->fdn_Title ) )
			{
				usbbase->usb_IExec->DebugPrintF ( "USB2 bind: '%s' %04lX:%04lX interface %lu start result $%08lX\n",
												  fdn->fdn_Title, (U32)LE_SWAP16 ( fn->fkt_DeviceDescriptor->VendorID ),
												  (U32)LE_SWAP16 ( fn->fkt_DeviceDescriptor->DeviceID ),
												  (U32)ifcdsc->InterfaceNumber, (U32)stat );
			}
#endif

			if ( ! myIS_TASKRETURN_ERR ( stat ) )
			{
				return ( TRUE );
			}
		}
	}

	return ( FALSE );
}

// --

static enum FDSTAT
_Check_for_Interfaces ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceHeader * interfaces[256];
	struct USB2_InterfaceGroup *  ig;
	struct USB2_ConfigNode *	  cn;
	enum FDSTAT					  stat;

	U32 count;
	U32 index;

	USBDEBUG ( "_Check_for_Interfaces: 1" );

	stat  = FDSTAT_Not_Found;
	count = 0;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	cn = fn->fkt_Config_Active;

	if ( ( cn ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) )
	{
		ig = cn->cfg_InterfaceGroups.uh_Head;

		while ( ig )
		{
			// An association group can contain interfaces with different classes.
			ih = ig->ig_Group.uh_Head;

			while ( INTERFACE_VALIDHEADER ( ih ) == VSTAT_Okay )
			{
				if ( INTERFACE_LOCKHEADER ( ih ) == LSTAT_Okay )
				{
					interfaces[count++] = ih;
				}

				ih = Node_Next ( ih );
			}

			ig = Node_Next ( ig );
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	for ( index = 0; index < count; index++ )
	{
		ih = interfaces[index];

		if ( _Check_Interface ( usbbase, fn, ih, as, NULL ) )
		{
			stat = FDSTAT_Found;
		}

		INTERFACE_UNLOCKHEADER ( ih );
	}

	return ( stat );
}

// --
