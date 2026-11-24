
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HUB.h"

// --

SEC_CODE static S32
PortReady( struct intern *in, const struct USB2_PortStatus *status )
{
	// The synthetic root hub exposes Enable only while the USB3 link is in U0.
	return ( ( status->wPortStatus & PORTSTATF_Enable ) && ( ! ( status->wPortStatus & PORTSTATF_Reset ) ) &&
		( ( ! in->Function->fkt_Tier ) || ( ! ( status->wPortStatus & PORTSTATF_SS_LINK_STATE ) ) ) );
}

// --

SEC_CODE enum HUB_AttachResult
HUB__Port_Attach_Super( struct USBBase *usbbase, struct intern *in, U32 port )
{
	struct USB2_PortStatus status;
	struct RealFunctionNode *fn;

	enum HUB_AttachResult result;

	U32 address;
	S32 err;

	result = HUB_Attach_Retry;
	fn = NULL;
	in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;

	err = HUB__Port_Status_Get( usbbase, in, port, &status );

	if ( err )
	{
		goto finished;
	}

	if ( ( ! PortReady( in, &status ) ) || ( in->PortWork[port].Attempts ) )
	{
		if ( ( status.wPortChange & PORTCHGF_Connection ) ||
			( ! ( status.wPortStatus & PORTSTATF_Connection ) ) )
		{
			goto check_connection;
		}

		if ( status.wPortChange & PORTCHGF_BH_PORT_RESET )
		{
			err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_BH_RESET );

			if ( err )
			{
				goto finished;
			}
		}

		err = HUB__Port_Feature_Set( usbbase, in, port, PORT_BH_RESET );

		if ( err )
		{
			goto finished;
		}

		do
		{
			HUB_DELAY_MS( 10 );

			err = HUB__Port_Status_Get( usbbase, in, port, &status );

			if ( err )
			{
				goto finished;
			}

			if ( ( status.wPortChange & PORTCHGF_Connection ) ||
				( ! ( status.wPortStatus & PORTSTATF_Connection ) ) )
			{
				goto check_connection;
			}
		}
		while ( ( ! PortReady( in, &status ) ) || ( ! ( status.wPortChange & PORTCHGF_BH_PORT_RESET ) ) );

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_BH_RESET );

		if ( err )
		{
			goto finished;
		}
	}

check_connection:
	if ( status.wPortChange & PORTCHGF_Connection )
	{
		result = HUB_Attach_Reconnect;
		goto finished;
	}

	if ( ! ( status.wPortStatus & PORTSTATF_Connection ) )
	{
		result = HUB_Attach_Gone;
		goto finished;
	}

	if ( ! HUB_Can_Enumerate( usbbase, in ) )
	{
		goto finished;
	}

	fn = HUB__Port_Function_Address( usbbase, in, USBSPEED_Super, port, &address );

	if ( ( fn ) && ( address ) )
	{
		in->ControlDeadline = HUB_Uptime( usbbase ) + 1500000;

		if ( HUB__Port_Function_SetAddr( usbbase, in, fn ) )
		{
			in->ControlDeadline = 0;

			if ( HUB__Port_Function_Configure( usbbase, in, fn, port, &status ) )
			{
				return ( HUB_Attach_Complete );
			}
		}
	}

finished:
	in->ControlDeadline = 0;

	if ( fn )
	{
		HUB__Port_Function_Discard( usbbase, in, fn );
	}

	return ( result );
}

// --
