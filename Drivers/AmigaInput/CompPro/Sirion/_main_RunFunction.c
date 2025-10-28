
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "CompPro.h"

// --

static const struct USB_Controller USB_Controller_IDs[] =
{
	{ "Competition Pro Game Controller",	0x0079,	0x181c,	0 },
	{ NULL,									0x0000,	0x0000,	0 }
};

S32 _main_RunFunction( struct USBFDIFace *Self, struct USBFDStartupMsg *msg )
{
struct USBBusDevDsc *dsc;
struct libBase *libBase;
S32 retval;
U32 cnt;

	libBase = (PTR) Self->Data.LibBase;

	cnt = 0;
	dsc = (PTR) msg->Descriptor;

	while( USB_Controller_IDs[cnt].vendor )
	{
		if (( LE_SWAP16( dsc->dd_VendorID ) == USB_Controller_IDs[cnt].vendor ) &&
			( LE_SWAP16( dsc->dd_Device  ) == USB_Controller_IDs[cnt].product ))
		{
			break;
		}
		else
		{
			cnt++;
		}
	}

	if ( USB_Controller_IDs[cnt].vendor )
	{
		#ifdef DEBUG
		DebugPrintF( "'%s' controller found\n", USB_Controller_IDs[cnt].name );
		#endif

		retval = Driver_Function( Self, msg, USB_Controller_IDs[cnt].type );
	}
	else
	{
		#ifdef DEBUG
		DebugPrintF( "Device Not supported\n" );
		DebugPrintF( "dd_VendorID: 0x%04x\n", LE_SWAP16( dsc->dd_VendorID ));
		DebugPrintF( "dd_Device:  0x%04x\n", LE_SWAP16( dsc->dd_Device ));
		#endif

		retval = USBERR_UNSUPPORTED;
	}

	return( retval );
}

// --
