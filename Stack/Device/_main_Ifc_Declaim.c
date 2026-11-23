
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

static S32
_main_Ifc_Declaim ( struct USB2IFace * Self, struct RealRegister * reg, struct USB2_Interface * ifc )
{
	struct USB2_InterfaceHeader * ih;
	struct USBBase *			  usbbase;

	S32 retval;

	usbbase = (PTR)Self->Data.LibBase;
	retval	= FALSE;

	if ( ( ! ifc ) || ( REGISTER_LOCK ( reg ) != LSTAT_Okay ) )
	{
		return ( FALSE );
	}

	ih = LockRegistrationInterface ( usbbase, reg, ifc );

	if ( ih )
	{
		INTERFACE_DECLAIMHEADER ( reg, ih );

		INTERFACE_UNLOCKHEADER ( ih );

		retval = TRUE;
	}

	REGISTER_UNLOCK ( reg );

	return ( retval );
}

// --
