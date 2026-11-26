
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE S32
USB2_Notify_SetIOFilter( struct USBBase *usbbase, const struct USB2_IOCaptureFilter *filter )
{
	struct USB2_NotifyNode *node;
	struct USB2_IOCaptureTarget *targets;
	struct USB2_IOCaptureTarget *old;

	U32 bytes;
	S32 result;

	if ( ( ! filter->Subscription ) || ( filter->Count > UINT32_MAX / sizeof( *targets ) ) ||
		( ( filter->Count ) && ( ! filter->Targets ) ) )
	{
		return ( FALSE );
	}

	targets = NULL;
	result = FALSE;

	if ( filter->Count )
	{
		bytes = filter->Count * sizeof( *targets );

		targets = MEM_ALLOCVEC( bytes, FALSE );

		if ( ! targets )
		{
			return ( FALSE );
		}

		MEM_COPY( (PTR) filter->Targets, targets, bytes );
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

	SEMAPHORE_OBTAIN( &usbbase->usb_Notify_Semaphore );

	for( node = usbbase->usb_Notify_Header.uh_Head; node; node = Node_Next( node ) )
	{
		if ( ( node == filter->Subscription ) && ( node->nn_Type == USBNT_IORequest ) )
		{
			old = node->nn_IOFilter;
			node->nn_IOFilter = targets;
			node->nn_IOFilterCount = filter->Count;
			targets = old;
			usbbase->usb_Notify_IOWatchers += ( node->nn_IOFilter != NULL ) - ( old != NULL );
			result = TRUE;
			break;
		}
	}

	SEMAPHORE_RELEASE( &usbbase->usb_Notify_Semaphore );

	SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

	if ( targets )
	{
		MEM_FREEVEC( targets );
	}

	return ( result );
}

// --
