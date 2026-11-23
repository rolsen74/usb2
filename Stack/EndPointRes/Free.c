
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

SEC_CODE static void
__Release ( struct USBBase * usbbase, struct RealEndPointResource * epr )
{
	struct RealRequest * ioreq;

	U32 count;
	U32 index;
	S32 active;

	if ( ! epr->epr_Public.IORequests )
	{
		return;
	}

	count = epr->epr_RequestCount;

	// The first request can own the reply port shared by the rest.
	while ( count )
	{
		index = --count;
		ioreq = (PTR)epr->epr_Public.IORequests[index];

		if ( ! ioreq )
		{
			continue;
		}

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		active = ( ioreq->req_PublicStat != IORS_User );

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		if ( active )
		{
			usbbase->usb_IExec->AbortIO ( (PTR)ioreq );

			// The reply port can signal another task.
			SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

			while ( ioreq->req_PublicStat != IORS_User )
			{
				SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

				MISC_WAIT ( 1000 );

				SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );
			}

			SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
		}

		while ( IOREQUEST_FREE ( ioreq ) == FSTAT_Locked )
		{
			MISC_WAIT ( 1000 );
		}

		epr->epr_Public.IORequests[index] = NULL;
	}

	MEM_FREEVEC ( epr->epr_Public.IORequests );

	epr->epr_Public.IORequests = NULL;
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE enum FSTAT
__EndPointRes_Free ( struct USBBase * usbbase, struct RealEndPointResource * epr, STR file UNUSED )

#else

SEC_CODE enum FSTAT
__EndPointRes_Free ( struct USBBase * usbbase, struct RealEndPointResource * epr )

#endif

{
	struct RealRegister * reg;

	enum VSTAT vstat;
	PTR		   buf;
	S32		   pos;

	USBDEBUG ( "__EndPointRes_Free       : EPR   %p : (%s)", epr, ( file ) ? file : "<NULL>" );

	// --

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	vstat = ENDPOINTRES_VALID ( epr );

	if ( vstat != VSTAT_Okay )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		return ( ( vstat == VSTAT_Null ) ? FSTAT_Null : FSTAT_Error );
	}

	epr->epr_FreeMe = TRUE;

	while ( epr->epr_Locks )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		MISC_WAIT ( 1000 );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	__Release ( usbbase, epr );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	epr->epr_StructID = ID_USB2_FREED;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	USBINFO ( "__EndPointRes_Free       : Freeing %p : (%s)", epr, ( file ) ? file : "<NULL>" );

	// --

#ifdef DO_PANIC

	if ( ( ( Node_Next ( epr ) ) && ( Node_Next ( epr ) != (PTR)0xAC111111 ) )
		 || ( ( Node_Prev ( epr ) ) && ( Node_Prev ( epr ) != (PTR)0xAC222222 ) ) )
	{
		USBPANIC ( "__EndPointRes_Free  : EndPointRes Node have not been removed : Self %p : Next %p : Prev %p : (%s)", epr,
				   Node_Next ( epr ), Node_Prev ( epr ), file );
	}

#endif

	// --

	if ( epr->epr_Public.Buffers )
	{
		pos = 0;

		while ( TRUE )
		{
			buf = epr->epr_Public.Buffers[pos];

			if ( ! buf )
			{
				break;
			}

			MEM_FREEIOBUFFER ( epr->epr_Public.Buffers[pos] );
			epr->epr_Public.Buffers[pos] = NULL;
			pos++;
		}

		MEM_FREEVEC ( epr->epr_Public.Buffers );
		epr->epr_Public.BufferSize = 0;
		epr->epr_Public.Buffers	   = NULL;
	}

	// --

	if ( epr->epr_Public.SetupData )
	{
		//		MEM_FREEIOBUFFER( epr->epr_Public.SetupData );
		SETUPDATA_FREE ( epr->epr_Public.SetupData );
		epr->epr_Public.SetupData = NULL;
	}

	// --

	if ( epr->epr_EndPoint )
	{
		ENDPOINT_UNLOCK ( epr->epr_EndPoint );
	}

	reg = epr->epr_Register;

	if ( reg )
	{
		REGISTER_UNLOCK ( reg );
		epr->epr_Register = NULL;
	}

	// --

	MEMORY_FREE ( MEMID_USBEndPointResource, epr, 0 );

	return ( FSTAT_Okay );
}

// --
