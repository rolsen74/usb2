
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// A registration can only claim interfaces on its own function.
static struct USB2_InterfaceHeader *
LockRegistrationInterface ( struct USBBase * usbbase, struct RealRegister * reg, const struct USB2_Interface * ifc )
{
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceHeader * found;
	struct USB2_InterfaceGroup *  ig;
	struct USB2_ConfigNode *	  cn;
	struct RealFunctionNode *	  fn;

	found = NULL;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	fn = reg->reg_Function;

	if ( fn )
	{
		for ( cn = fn->fkt_Configs.uh_Head; ( cn ) && ( ! found ); cn = Node_Next ( cn ) )
		{
			for ( ig = cn->cfg_InterfaceGroups.uh_Head; ( ig ) && ( ! found ); ig = Node_Next ( ig ) )
			{
				for ( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next ( ih ) )
				{
					if ( ( &ih->ih_Public == ifc ) && ( INTERFACE_LOCKHEADER ( ih ) == LSTAT_Okay ) )
					{
						found = ih;
						break;
					}
				}
			}
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	return ( found );
}

// --

static S32
_main_Ifc_Claim ( struct USB2IFace * Self, struct RealRegister * reg, struct USB2_Interface * ifc )
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
		retval = INTERFACE_CLAIMHEADER ( reg, ih );

		INTERFACE_UNLOCKHEADER ( ih );
	}

	REGISTER_UNLOCK ( reg );

	return ( retval );
}

// --
