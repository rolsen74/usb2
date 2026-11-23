
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE enum FSTAT
__EndPointRes_Release ( struct USBBase * usbbase, struct USB2_EPResource * epr, STR file UNUSED )

#else

SEC_CODE enum FSTAT
__EndPointRes_Release ( struct USBBase * usbbase, struct USB2_EPResource * epr )

#endif

{
	struct RealEndPointResource * real;
	struct RealRegister *		  reg;

	enum FSTAT stat;

	USBDEBUG ( "__EndPointRes_Release : EPR   %p : (%s)", epr, ( file ) ? file : "<NULL>" );

	if ( ! epr )
	{
		return ( FSTAT_Null );
	}

	real = (PTR)( (char *)epr - offsetof ( struct RealEndPointResource, epr_Public ) );
	stat = FSTAT_Null;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	if ( ENDPOINTRES_VALID ( real ) == VSTAT_Okay )
	{
		if ( real->epr_FreeMe )
		{
			stat = FSTAT_Locked;
		}
		else if ( ( real->epr_ReplyOwner ) && ( real->epr_ReplyOwner != TASK_FIND () ) )
		{
			usbbase->usb_IExec->DebugPrintF ( "USB2: release endpoint resource on its reply-port task\n" );

			stat = FSTAT_Error;
		}
		else
		{
			real->epr_FreeMe = TRUE;
			reg				 = real->epr_Register;

			if ( reg )
			{
				if ( reg->reg_Public.Res_Control == epr )
				{
					reg->reg_Public.Res_Control = NULL;
				}

				NODE_REMNODE ( &reg->reg_EPRHeader, &real->epr_Node );
			}

			stat = FSTAT_Okay;
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( stat == FSTAT_Okay )
	{
		stat = ENDPOINTRES_FREE ( real );
	}

	return ( stat );
}

// --
