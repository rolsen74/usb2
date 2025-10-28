
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

SEC_CODE static void __Free_3_Rem_Int( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct intern *in UNUSED )
{
	if ( hn->hn_Reset_Interrupt_Added )
	{
		MISC_REMRESETCALLBACK( & hn->hn_Reset_Interrupt );
		hn->hn_Reset_Interrupt_Added = FALSE;
	}

	if ( hn->HCD_Interrupt_Added )
	{
		MISC_REMINTSERVER( hn->hn_HCDInterruptNr, & hn->HCD_Interrupt );
		hn->HCD_Interrupt_Added = FALSE;
	}
}

// --
