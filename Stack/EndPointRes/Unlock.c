
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void __EndPointRes_Unlock( struct USBBase *usbbase, struct RealEndPointResource *epr, STR file UNUSED )

#else

SEC_CODE void __EndPointRes_Unlock( struct USBBase *usbbase, struct RealEndPointResource *epr )

#endif

{
	TASK_NAME_ENTER( "__EndPointRes_Unlock" );

	USBDEBUG( "__EndPointRes_Unlock     : EPR %p : Locks %ld (old) : (%s)", epr, (epr)?epr->epr_Locks:0, (file)?file:"<NULL>" );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( ENDPOINTRES_VALID(epr) == VSTAT_Okay )
	{
		if ( epr->epr_Locks > 0 )
		{
			epr->epr_Locks--;

//			USBDEBUG( "__EndPointRes_Unlock     : EPR %p : New Lock Count : %ld (-1)", epr, epr->epr_Locks );
		}
		else
		{
			USBPANIC( "__EndPointRes_Unlock     : EPR %p : EndPoint Resource NOT Locked (%ld)", epr, epr->epr_Locks );
		}
	}
	else
	{
		USBPANIC( "__EndPointRes_Unlock     : EPR %p : Invalid EndPointRes Node", epr );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	TASK_NAME_LEAVE();
}

// --
