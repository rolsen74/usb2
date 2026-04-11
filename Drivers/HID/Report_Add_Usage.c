
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE S32 Report_Add_Usage( struct USBBase *usbbase, struct HIDData *hd UNUSED, struct USB2_Header *list, U32 minusage, U32 maxusage )
{
struct HID_UsageNode *un;
S32 error;


	error = TRUE;

	if ( minusage > maxusage )
	{
		USBDEBUG( "Min %08lx > Max %08lx", minusage, maxusage );
		goto bailout;
	}

	un = Node_Usage_Alloc( usbbase );

	if ( ! un )
	{
		USBERROR( "Error allocting memory" );
		goto bailout;
	}

	un->Min	= minusage;
	un->Max	= maxusage;

	NODE_ADDTAIL( list, un );

	error = FALSE;

bailout:

	return(	error );
}
