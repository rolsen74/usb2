
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static enum FDSTAT
_Check_for_Devices ( struct USBBase * usbbase, struct RealFunctionNode * fn, struct USB2_ASync * as )
{
	struct USB2_FktDriverNode * fdn;

	U32 result;

	if ( ! fn->fkt_Tier )
	{
		return ( FDSTAT_Not_Found );
	}

	for ( fdn = usbbase->usb_FDriver_Header.uh_Head; fdn; fdn = Node_Next ( fdn ) )
	{
		if ( ( fdn->fdn_Type != USB2Val_FDriver_Device ) || ( fdn->fdn_Detach )
			 || ( ( fdn->fdn_Driver_Filename ) && ( ! usbbase->usb_FrontendsReady ) ) )
		{
			continue;
		}

		if ( Match_Driver_Device ( usbbase, fdn, fn->fkt_DeviceDescriptor ) != DriverMatch_ID )
		{
			continue;
		}

		result = _Start_Function ( usbbase, fn, fdn, as );

		if ( ( result == TASK_Return_Okay ) || ( result == TASK_Return_Stack_Okay ) )
		{
			return ( FDSTAT_Found );
		}
	}

	return ( FDSTAT_Not_Found );
}

// --
