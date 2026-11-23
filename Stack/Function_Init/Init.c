
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

struct FunctionInit
{
	struct RealFunctionNode *fn;
	S32 Success;
};

// --

SEC_CODE static S32
FunctionAttached( struct RealFunctionNode *fn )
{
	struct RealFunctionNode *parent;

	parent = fn->fkt_Parent;

	return ( ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) &&
		( ( ! parent ) || ( ( ! parent->fkt_Detach ) && ( ! parent->fkt_FreeMe ) ) ) );
}

// --

SEC_CODE static S32
InitCancelled( struct USBBase *usbbase, struct RealFunctionNode *fn, S32 error )
{
	S32 attached;

	if ( ( error == USB2Err_Device_Detached ) || ( error == IOERR_ABORTED ) )
	{
		return ( TRUE );
	}

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	attached = FunctionAttached( fn );

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	return ( ! attached );
}

// --

// The caller holds the stack lock and has completed initialization.
SEC_CODE S32
USB2_Function_Publish( struct USBBase *usbbase, struct RealFunctionNode *fn )
{
	if ( ! FunctionAttached( fn ) )
	{
		return ( FALSE );
	}

	if ( ! fn->fkt_Initialized )
	{
		fn->fkt_Initialized = TRUE;

		USB2_Notify_Function( usbbase, fn, USBNC_Adding );
	}

	return ( TRUE );
}

// --

#include "Init_2_Main.c"

// --

SEC_CODE S32
__Function_Init( struct USBBase *usbbase, struct RealFunctionNode *fn, struct USB2_ASync *as )
{
	struct FunctionInit init;

	if ( FUNCTION_LOCK( fn ) != LSTAT_Okay )
	{
		USBERROR( "__Function_Init : Function Lock Error" );

		return ( FALSE );
	}

	init.fn = fn;
	init.Success = fn->fkt_Initialized;

	if ( ! init.Success )
	{
		TASK_START(
			USB2_WaitForTermination, TRUE,
			TASK_Tag_Func_Main, __myMain,
			TASK_Tag_UserData, & init,
			TASK_Tag_ASync, as,
			TASK_Tag_Type, TASK_Type_FKT,
			TASK_Tag_Name, "Device Init",
			TAG_END
		);
	}

	FUNCTION_UNLOCK( fn );

	return ( init.Success );
}

// --
