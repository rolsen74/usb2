
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include <interfaces/usb2eth.h>

// --

S32
WiFi_LiveRegistration( U32 unit, ULONG *registration )
{
	struct USB2EthIFace *ethernet;
	struct IORequest request;

	S32 found;

	memset( &request, 0, sizeof( request ) );

	request.io_Message.mn_Length = sizeof( request );
	*registration = 0;

	if ( OpenDevice( USB2_ETH_DEVICE, USB2_ETH_BACKEND_UNIT, &request, 0 ) )
	{
		return ( FALSE );
	}

	found = TRUE;

	if ( request.io_Device->dd_Library.lib_Revision >= USB2ETH_REGISTRATION_REVISION )
	{
		ethernet = (APTR) GetInterface( (APTR) request.io_Device, "main", 1, NULL );
		found = FALSE;

		if ( ethernet )
		{
			*registration = ethernet->GetRegistrationID( NULL, unit );

			found = *registration != 0;

			DropInterface( (APTR) ethernet );
		}
	}

	CloseDevice( &request );

	return ( found );
}

// --
