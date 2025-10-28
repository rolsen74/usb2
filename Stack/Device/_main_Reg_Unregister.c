
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
/* -- Main Interface -- */

static void _main_Reg_Unregister( struct USB2_IFace *Self, struct RealRegister *reg )
{
struct USBBase *usbbase;

	usbbase = (PTR) Self->Data.LibBase;
	USBERROR( "USB2 Stack : _main_Reg_Unregister" );

	if ( REGISTER_VALID( reg ) != VSTAT_Okay )
	{
		USBERROR( "Error invalid Register Pointer" );
		goto bailout;
	}

	REGISTER_UNREGISTER( reg );

bailout:

	return;
}

// --

