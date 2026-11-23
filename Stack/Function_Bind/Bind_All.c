
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static struct RealFunctionNode *
Next_Enumerated( struct USBBase *usbbase, struct RealFunctionNode *previous )
{
	struct RealFunctionNode *fn;
	struct USB2_Node *node;

	fn = NULL;

	SEMAPHORE_OBTAIN( & usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( previous )
	{
		node = previous->fkt_GlobalNode.un_Next;

		FUNCTION_UNLOCK( previous );
	}
	else
	{
		node = Header_Head( & usbbase->usb_Fkt_Header );
	}

	while ( node )
	{
		fn = node->un_Data;

		if ( ( FUNCTION_VALID( fn ) == VSTAT_Okay ) && ( fn->fkt_HUB_Data ) &&
			( fn->fkt_Initialized ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) &&
			( FUNCTION_LOCK( fn ) == LSTAT_Okay ) )
		{
			break;
		}

		fn = NULL;
		node = node->un_Next;
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE( & usbbase->usb_Fkt_Semaphore );

	return ( fn );
}

// --

SEC_CODE void
USB2_Bind_Enumerated( struct USBBase *usbbase )
{
	struct RealFunctionNode *fn;

	fn = Next_Enumerated( usbbase, NULL );

	while ( fn )
	{
		FUNCTION_BIND( fn, fn->fkt_DriverASync );

		fn = Next_Enumerated( usbbase, fn );
	}
}

// --
