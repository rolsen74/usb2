
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

SEC_CODE void
__Task_Stop_FN ( struct USBBase * usbbase, struct RealFunctionNode * fn, STR file UNUSED )

#else

SEC_CODE void
__Task_Stop_FN ( struct USBBase * usbbase, struct RealFunctionNode * fn )

#endif

{
	struct USB2_DriverNode * dn;
	struct USB2_DriverNode * stop;

	if ( FUNCTION_LOCK ( fn ) != LSTAT_Okay )
	{
		return;
	}

	while ( TRUE )
	{
		stop = NULL;

		SEMAPHORE_OBTAIN ( &usbbase->usb_Bind_Semaphore );

		for ( dn = usbbase->usb_Bind_Header.uh_Head; dn; dn = Node_Next ( dn ) )
		{
			if ( ( dn->dn_Function == fn ) && ( ! dn->dn_Detach ) )
			{
				dn->dn_Detach = TRUE;

				if ( ( dn->dn_Task ) && ( DRIVER_LOCK ( dn ) == LSTAT_Okay ) )
				{
					stop = dn;
					break;
				}
			}
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_Bind_Semaphore );

		if ( ! stop )
		{
			break;
		}

		TASK_STOP_DN ( stop );

		DRIVER_UNLOCK ( stop );
	}

	FUNCTION_UNLOCK ( fn );
}

// --
