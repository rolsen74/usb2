
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE static void __myTask_Free( struct USBBase *usbbase, struct intern *in )
{
	USBINFO( "__myTask_Free (Master)" );

	__myTask_Def_Free( usbbase, in );
}

// --
