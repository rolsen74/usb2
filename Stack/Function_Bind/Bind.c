
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// #include "Bind__Entry.c"

enum DriverDeviceMatch
{
	DriverMatch_None,
	DriverMatch_Class,
	DriverMatch_ID
};

// --

SEC_CODE static enum DriverDeviceMatch
Match_Device_IDs ( U16 vendor, U16 product, U16 filter_vendor, U16 filter_product )
{
	if ( ( ( filter_vendor ) && ( filter_vendor != vendor ) ) || ( ( filter_product ) && ( filter_product != product ) ) )
	{
		return ( DriverMatch_None );
	}

	return ( ( ( filter_vendor ) || ( filter_product ) ) ? DriverMatch_ID : DriverMatch_Class );
}

// --

SEC_CODE static enum DriverDeviceMatch
Match_Driver_Device ( struct USBBase * usbbase, struct USB2_FktDriverNode * fdn, const struct USB2_Device_Desc * desc )
{
	struct USB2_DeviceNode * entry;
	struct ExecIFace *		 IExec;
	enum DriverDeviceMatch	 match;
	enum DriverDeviceMatch	 best;

	U16 vendor;
	U16 product;

	IExec	= usbbase->usb_IExec;
	vendor	= LE_SWAP16 ( desc->VendorID );
	product = LE_SWAP16 ( desc->DeviceID );
	best	= DriverMatch_None;

	entry = (PTR)IExec->GetHead ( &fdn->fdn_Devices );

	if ( ! entry )
	{
		return ( Match_Device_IDs ( vendor, product, fdn->fdn_VendorID, fdn->fdn_DeviceID ) );
	}

	while ( entry )
	{
		match = Match_Device_IDs ( vendor, product, entry->VendorID, entry->DeviceID );

		if ( match == DriverMatch_ID )
		{
			return ( match );
		}

		if ( match == DriverMatch_Class )
		{
			best = match;
		}

		entry = (PTR)IExec->GetSucc ( &entry->Node );
	}

	return ( best );
}

// --

SEC_CODE static S32
DriverWorkerInit ( struct USBBase * usbbase, struct USB2_DriverNode * dn, PTR in UNUSED )
{
	S32 window;

	if ( DRIVER_LOCK ( dn ) != LSTAT_Okay )
	{
		return ( FALSE );
	}

	dn->dn_WorkerPinned = TRUE;

	if ( dn->dn_Entry )
	{
		return ( TRUE );
	}

	if ( ( ! dn->dn_Filename ) || ( ! usbbase->usb_DriverDirLock ) )
	{
		return ( FALSE );
	}

	window = MISC_SETPROCWINDOW ( -1 );

	dn->dn_OldDir	  = MISC_SETCURRENTDIR ( usbbase->usb_DriverDirLock );
	dn->dn_DirChanged = TRUE;

	dn->dn_Library = MISC_OPENLIBRARY ( dn->dn_Filename, 0 );

	if ( dn->dn_Library )
	{
		dn->dn_DriverIFace = MISC_OBTAININTERFACE ( dn->dn_Library, "main", 1 );
	}

	MISC_SETPROCWINDOW ( window );

	return ( ( dn->dn_DriverIFace ) && ( dn->dn_DriverIFace->Entry ) );
}

// --

SEC_CODE static void RetryDriverBinding ( struct USBBase * usbbase, struct USB2_DriverNode * dn );

// --

SEC_CODE static void
DriverWorkerMain ( struct USBBase * usbbase, struct USB2_DriverNode * dn, PTR in UNUSED )
{
	struct Task * parent;

	U32 result;

	if ( dn->dn_Entry )
	{
		result = dn->dn_Entry ( usbbase, &dn->dn_Message.rdm_Public );
	}
	else
	{
		result = dn->dn_DriverIFace->Entry ( &dn->dn_Message.rdm_Public );
	}

	// The root hub can report readiness before entering its event loop.
	parent = dn->dn_Message.rdm_Parent;

	if ( parent )
	{
		dn->dn_Message.rdm_Parent = NULL;
		dn->dn_Message.rdm_Result = myGET_TASKRETURN_CODE ( result );

		TASK_SIGNAL ( parent, dn->dn_Message.rdm_Mask );
	}
	else if ( ( ( dn->dn_Interface ) || ( dn->dn_Function->fkt_Tier ) )
			  && ( ( myIS_TASKRETURN_ERR ( result ) ) || ( result == TASK_Return_Unsupported ) ) )
	{
		RetryDriverBinding ( usbbase, dn );
	}
}

// --

SEC_CODE static void
DriverWorkerFree ( struct USBBase * usbbase, struct USB2_DriverNode * dn, PTR in UNUSED )
{
	if ( dn->dn_DriverIFace )
	{
		MISC_RELEASEINTERFACE ( dn->dn_DriverIFace );
	}

	if ( dn->dn_Library )
	{
		MISC_CLOSELIBRARY ( dn->dn_Library );
	}

	if ( dn->dn_DirChanged )
	{
		MISC_SETCURRENTDIR ( dn->dn_OldDir );
	}

	if ( dn->dn_WorkerPinned )
	{
		dn->dn_FreeMe = TRUE;

		DRIVER_UNLOCK ( dn );
	}
}

// --

// Consume the caller's retained task reference.
SEC_CODE static S32
PublishDriver ( struct USBBase * usbbase, struct USB2_DriverNode * dn, struct USB2_TaskNode * tn )
{
	S32 detached;
	S32 stop;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Bind_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	detached	  = ( dn->dn_Function->fkt_Detach ) || ( dn->dn_Function->fkt_FreeMe );
	dn->dn_Detach = detached;
	dn->dn_Task	  = tn;
	tn->tn_Owner  = dn;
	stop		  = ( detached ) && ( TASK_LOCK ( tn ) == LSTAT_Okay );

	NODE_ADDTAIL ( &usbbase->usb_Bind_Header, dn );

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Bind_Semaphore );

	if ( stop )
	{
		TASK_STOP_TN ( tn );

		TASK_UNLOCK ( tn );
	}

	return ( ! detached );
}

// --

SEC_CODE static void
DiscardDriver ( struct USBBase * usbbase, struct USB2_DriverNode * dn )
{
	if ( DRIVER_FREE ( dn ) != FSTAT_Okay )
	{
		SEMAPHORE_OBTAIN ( &usbbase->usb_Bind_Semaphore );

		NODE_ADDTAIL ( &usbbase->usb_Bind_Header, dn );

		SEMAPHORE_RELEASE ( &usbbase->usb_Bind_Semaphore );
	}
}

// --

// Class records are immutable until the workers and scanner have stopped.
SEC_CODE static S32
ClassAvailable ( struct USBBase * usbbase, const struct USB2_FktDriverNode * fdn )
{
	return ( ( ! fdn->fdn_Detach ) && ( ( ! fdn->fdn_Driver_Filename )
			 || ( ( usbbase->usb_FrontendsReady ) && ( usbbase->usb_ClassesReady ) ) ) );
}

// --

#include "Bind_Check_for_Function.c"
#include "Bind_Check_for_Device.c"
#include "Bind_Check_for_Interfaces.c"

// --

// Called with the class registry locked.
SEC_CODE static enum FDSTAT
BindClasses ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as,
			  struct USB2_DriverNode * previous )
{
	struct USB2_DriverNode * bound;
	struct USB2_FktDriverNode * after;

	enum FDSTAT stat;
	S32 available;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Bind_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	available = ( ! fn->fkt_Owner ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe );

	for ( bound = Header_Head ( &usbbase->usb_Bind_Header ); ( available ) && ( bound ); bound = Node_Next ( bound ) )
	{
		if ( ( bound != previous ) && ( bound->dn_Function == fn ) && ( ! bound->dn_Interface )
			 && ( ! bound->dn_Detach ) && ( ! bound->dn_FreeMe ) )
		{
			available = FALSE;
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Bind_Semaphore );

	if ( ! available )
	{
		return ( FDSTAT_Not_Found );
	}

	after = previous ? previous->dn_Class : NULL;
	stat = FDSTAT_Not_Found;

	if ( ( ! after ) || ( after->fdn_Type == USB2Val_FDriver_Device ) )
	{
		stat = _Check_for_Devices ( usbbase, fn, as, after );
	}

	if ( stat == FDSTAT_Not_Found )
	{
		stat = _Check_for_Functions ( usbbase, fn, as,
			( ( after ) && ( after->fdn_Type == USB2Val_FDriver_Function ) ) ? after : NULL );
	}

	if ( stat == FDSTAT_Not_Found )
	{
		stat = _Check_for_Interfaces ( usbbase, fn, as );
	}

	return ( stat );
}

// --

SEC_CODE static void
RetryDriverBinding ( struct USBBase * usbbase, struct USB2_DriverNode * dn )
{
	SEMAPHORE_OBTAIN ( &usbbase->usb_FDriver_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	// The worker still pins this node while the next probe starts.
	dn->dn_FreeMe = TRUE;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( dn->dn_Interface )
	{
		_Check_Interface ( usbbase, dn->dn_Function, dn->dn_Interface, dn->dn_ASync, dn );
	}
	else
	{
		BindClasses ( usbbase, dn->dn_Function, dn->dn_ASync, dn );
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_FDriver_Semaphore );
}

// --

SEC_CODE S32
__Function_Bind ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as )
{
	enum FDSTAT stat;
	S32			retval;

#ifdef USB2_BIND_DEBUG
	U32 config;
#endif

	//	usbbase->usb_IExec->DebugPrintF( "\n##\n## __Function_Bind : FN    %p\n##\n\n", fn );

	// --

#ifdef DO_PANIC

	if ( TASK_FIND () == usbbase->usb_Master_Task.tn_TaskAdr )
	{
		// if run from master task, we will get a dead lock when starting a driver
		USBPANIC ( "__Function_Bind          : Do NOT run from Master Task" );
	}

#endif

	// --

	stat = FDSTAT_Not_Found;

	if ( FUNCTION_LOCK ( fn ) == LSTAT_Okay )
	{
#ifdef USB2_BIND_DEBUG
		if ( fn->fkt_DeviceDescriptor )
		{
			SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

			config = ( fn->fkt_Config_Active ) ? fn->fkt_Config_Active->cfg_Number : 0;

			SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

			usbbase->usb_IExec->DebugPrintF ( "USB2 bind: %04lX:%04lX address %lu, active configuration %lu, owner %p\n",
											  (U32)LE_SWAP16 ( fn->fkt_DeviceDescriptor->VendorID ),
											  (U32)LE_SWAP16 ( fn->fkt_DeviceDescriptor->DeviceID ), (U32)fn->fkt_Address,
											  config, fn->fkt_Owner );
		}
#endif

		SEMAPHORE_OBTAIN ( &usbbase->usb_FDriver_Semaphore );

		stat = BindClasses ( usbbase, fn, as, NULL );

		SEMAPHORE_RELEASE ( &usbbase->usb_FDriver_Semaphore );

		FUNCTION_UNLOCK ( fn );
	}

	retval = ( stat != FDSTAT_Error ) ? TRUE : FALSE;

	//	usbbase->usb_IExec->DebugPrintF( "\n##\n## __Function_Bind 222 : Bind State %ld\n##\n\n", (S32) retval );

	return ( retval );
}

// --
