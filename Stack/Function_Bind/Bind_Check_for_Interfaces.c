
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void __myIFC_Entry( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
struct USB2DriverIFace *ifc;
struct USB2_DriverNode *dn;
PTR base;
S32 proc;
S32 old;

	TASK_NAME_ENTER( "__myIFC_Entry" );

	dn = userdata;

//	USBDEBUG( "__myIFCT_Entry : 1 : DN    %p : Call Driver", dn );

	// --

	if ( DRIVER_LOCK( dn ) == LSTAT_Okay )
	{
		/**/ if ( dn->dn_Entry )
		{
//			USBERROR( "__myIFC_Entry : 2 : DN    %p : Entry %p", dn, dn->dn_Entry );

			dn->dn_Entry( usbbase, & dn->dn_Message.rdm_Public );
		}
		else if (( dn->dn_Filename ) && ( usbbase->usb_DriverDirLock ))
		{
//			USBERROR( "__myIFC_Entry : 3 : DN    %p : Filename '%s'", dn, dn->dn_Filename );

			proc = MISC_SETPROCWINDOW( -1 );

//			USBERROR( "lock : $%08lx", usbbase->usb_DriverDirLock );

			old  = MISC_SETCURRENTDIR( usbbase->usb_DriverDirLock );

//			USBERROR( "old  : $%08lx", old );

			base = MISC_OPENLIBRARY( dn->dn_Filename, 0 );

			USBERROR( "base : %p", base );

			ifc	 = MISC_OBTAININTERFACE( base, "main", 1 );

			USBERROR( "ifc  : %p", ifc );

			MISC_SETPROCWINDOW( proc );

			if ( ifc )
			{
				ifc->Entry( & dn->dn_Message.rdm_Public );
			}

			MISC_RELEASEINTERFACE( ifc );

			MISC_CLOSELIBRARY( base );

			MISC_SETCURRENTDIR( old );
		}
		else
		{
			USBERROR( "__myIFC_Entry : 4 : DN    %p : No Entry", dn );
		}

		// Driver Quit
		dn->dn_FreeMe = TRUE;
	
		DRIVER_UNLOCK( dn );
	}

	// --

//	USBERROR( "__myIFC_Entry : 5 : DN    %p : Driver Exit", dn );

	TASK_NAME_LEAVE();
}

// --

SEC_CODE static S32 _Start_Interface( 
	struct USBBase *usbbase, 
	struct RealFunctionNode *fn,
	struct USB2_FktDriverNode *fdn,
	struct USB2_InterfaceHeader *ih,
	struct USB2_ASync *as )
{
struct USB2_DriverNode *dn;
struct USB2_TaskNode *tn;
enum TaskReturn stat;
U32 retval;

	TASK_NAME_ENTER( "_Start_Interface" );

	USBDEBUG( "_Start_Interface" );

	retval = TASK_Return_Stack_Error;

	dn = DRIVER_ALLOC( fn, as );

	if ( ! dn )
	{
		USBERROR( "_Start_Interface : Error allocating memory" );
		goto bailout;
	}

	dn->dn_Entry = fdn->fdn_Entry;
	dn->dn_Filename = fdn->fdn_Driver_Filename;

	// -- Init Public Message

	// Alloc check FN
//	if ( FUNCTION_VALID(fn) != VSTAT_Okay )
//	{
//		USBERROR( "_Start_Interface : Function Valid error (%p)", fn );
//		goto bailout;
//	}

	dn->dn_Message.rdm_Public.IUSB2 = usbbase->usb_IUSB2;
	dn->dn_Message.rdm_Public.VendorID = LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID );
	dn->dn_Message.rdm_Public.DeviceID = LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID );
	dn->dn_Message.rdm_Public.Function = (PTR) fn;
	dn->dn_Message.rdm_Public.ConfigDescriptors = (PTR) fn->fkt_Config_Desc_Buf;
	
	// --

	if ( INTERFACE_VALIDHEADER(ih) != VSTAT_Okay )
	{
		USBERROR( "_Start_Interface : Interface Header Valid error (%p)", ih );
		goto bailout;
	}

	#ifdef DO_DEBUG

	if ( ! ih->ih_Active )
	{
		USBPANIC( "_Start_Interface : ih_Active not set" );
	}

	#endif

	if ( ih->ih_Owner )
	{
		// Allready claimed
//		USBERROR( "Skipping Interface (claimed) : IH    %p", ih );
		goto bailout;
	}

//	USBERROR( "Found Interface (possible) : IH    %p", ih );

	dn->dn_Message.rdm_Public.Interface = (PTR) & ih->ih_Public ;
	dn->dn_Message.rdm_Public.InterfaceDescriptor = (PTR) ih->ih_Active->in_Descriptor;

	// -- Start Driver

	stat = TASK_START(
		TASK_Tag_Func_Main, __myIFC_Entry,
		TASK_Tag_UserData, dn,
		TASK_Tag_Get_TN, & tn,
		TASK_Tag_Prioity, fdn->fdn_Pri,
		TASK_Tag_ASync, as,
		TASK_Tag_Type, TASK_Type_FKT,
		TASK_Tag_Name, fdn->fdn_Title,
		TAG_END	
	);

	// --

	retval = stat;

	if ( TASKRETURN_OK(stat) )
	{
		USBDEBUG( "_Start_Interface         : Okay (Stat $%08lx)", stat );

		// --
		// -- Link Driver and Task

		if ( TASK_LOCK(tn) != LSTAT_Okay )
		{
			USBPANIC( "_Start_Interface : Invalid TN %p", tn );
		}

		#ifdef DO_DEBUG

		if ( tn->tn_Owner )
		{
			USBPANIC( "_Start_Interface : Already set tn_Owner  %p", tn->tn_Owner );
		}

		if ( dn->dn_Task )
		{
			USBPANIC( "_Start_Interface : Already set dn_Task  %p", dn->dn_Task );
		}

		#endif

		tn->tn_Owner = dn;
		dn->dn_Task = tn;

		// --

		SEMAPHORE_OBTAIN(	& usbbase->usb_Bind_Semaphore );
		NODE_ADDTAIL(		& usbbase->usb_Bind_Header, dn );
		SEMAPHORE_RELEASE(	& usbbase->usb_Bind_Semaphore );

//		retval = stat;
	}
	else
	{
		USBERROR( "_Start_Interface         : Failed (Stat $%08lx)", stat );

		if ( DRIVER_FREE( dn ) != FSTAT_Okay )
		{
			USBDEBUG( "_Start_Interface         : Unable to Free, parking" );

			SEMAPHORE_OBTAIN(	& usbbase->usb_Bind_Semaphore );
			NODE_ADDTAIL(		& usbbase->usb_Bind_Header, dn );
			SEMAPHORE_RELEASE(	& usbbase->usb_Bind_Semaphore );
		}

		dn = NULL;

//		retval = stat;
	}
  
	// --

bailout:

	TASK_NAME_LEAVE();

	return( retval );
}

// --

SEC_CODE static S32
_Check_Interface( struct USBBase *usbbase, struct RealFunctionNode *fn,
	struct USB2_InterfaceHeader *ih, struct USB2_ASync *as )
{
	struct USB2_FktDriverNode *fdn;
	struct USB2_Interface_Desc *ifcdsc;
	enum DriverDeviceMatch wanted;

	S32 stat;

	ifcdsc = ih->ih_Active ? ih->ih_Active->in_Descriptor : NULL;

	if ( ! ifcdsc )
	{
		return ( FALSE );
	}

	// Try device-specific classes first, keeping the existing order within each pass.
	for( wanted = DriverMatch_ID; wanted != DriverMatch_None; wanted-- )
	{
		for( fdn = Header_Head( & usbbase->usb_FDriver_Header ); fdn; fdn = Node_Next( fdn ) )
		{
			if ( ( fdn->fdn_Type != USB2Val_FDriver_Interface ) || ( fdn->fdn_Detach ) ||
				( ( fdn->fdn_Driver_Filename ) && ( ! usbbase->usb_FrontendsReady ) ) )
			{
				continue;
			}

			if ( Match_Driver_Device( usbbase, fdn, fn->fkt_DeviceDescriptor ) != wanted )
			{
				continue;
			}

			#ifdef USB2_BIND_DEBUG
			if ( ( wanted == DriverMatch_ID ) && ( fdn->fdn_Driver_Filename ) )
			{
				usbbase->usb_IExec->DebugPrintF( "USB2 bind: '%s' ID match, interface %lu %02lX/%02lX/%02lX, filter %lX/%lX/%lX\n",
					fdn->fdn_Driver_Filename, (U32) ifcdsc->InterfaceNumber, (U32) ifcdsc->InterfaceClass,
					(U32) ifcdsc->InterfaceSubClass, (U32) ifcdsc->InterfaceProtocol,
					(U32) fdn->fdn_Class, (U32) fdn->fdn_SubClass, (U32) fdn->fdn_Protocol );
			}
			#endif

			if ( ( ( fdn->fdn_Class != ifcdsc->InterfaceClass ) && ( fdn->fdn_Class != FDCLASS_Any ) ) ||
				( ( fdn->fdn_SubClass != ifcdsc->InterfaceSubClass ) && ( fdn->fdn_SubClass != FDSUBCLASS_Any ) ) ||
				( ( fdn->fdn_Protocol != ifcdsc->InterfaceProtocol ) && ( fdn->fdn_Protocol != FDPROTOCOL_Any ) ) )
			{
				continue;
			}

			stat = _Start_Interface( usbbase, fn, fdn, ih, as );

			#ifdef USB2_BIND_DEBUG
			if ( fdn->fdn_Driver_Filename )
			{
				usbbase->usb_IExec->DebugPrintF( "USB2 bind: '%s' interface %lu start result $%08lX\n",
					fdn->fdn_Driver_Filename, (U32) ifcdsc->InterfaceNumber, (U32) stat );
			}
			else if ( ( fdn->fdn_Entry ) && ( fdn->fdn_Title ) )
			{
				usbbase->usb_IExec->DebugPrintF( "USB2 bind: '%s' %04lX:%04lX interface %lu start result $%08lX\n",
					fdn->fdn_Title, (U32) LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID ),
					(U32) LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID ),
					(U32) ifcdsc->InterfaceNumber, (U32) stat );
			}
			#endif

			if ( ! myIS_TASKRETURN_ERR( stat ) )
			{
				return ( TRUE );
			}
		}
	}

	return ( FALSE );
}

// --

static enum FDSTAT _Check_for_Interfaces( struct USBBase *usbbase, struct RealFunctionNode *fn, struct USB2_ASync *as )
{
struct USB2_InterfaceHeader *ih;
struct USB2_InterfaceGroup *ig;
struct USB2_ConfigNode *cn;
enum FDSTAT stat;

	USBDEBUG( "_Check_for_Interfaces: 1" );

	stat = FDSTAT_Not_Found;

	cn = fn->fkt_Config_Active;

	if ( cn )
	{
		ig = cn->cfg_InterfaceGroups.uh_Head;

		while( ig )
		{
			// An association group can contain interfaces with different classes.
			ih = ig->ig_Group.uh_Head;

			while( INTERFACE_VALIDHEADER(ih) == VSTAT_Okay )
			{
				if ( _Check_Interface( usbbase, fn, ih, as ))
				{
					stat = FDSTAT_Found;
				}

				ih = Node_Next( ih );
			}

			ig = Node_Next( ig );
		}
	}

	return( stat );
}

// --
