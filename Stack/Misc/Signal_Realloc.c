
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

/*
** Running under Disable() and must not fail.
*/

SEC_CODE void __Task_ReallocSignal( struct USBBase *usbbase, struct USB2_Signal *sig )
{
struct ExecIFace *IExec;
S32 signal;

	#ifdef DO_PANIC

	if ( ! sig )
	{
		USBPANIC( "__Task_ReallocSignal : 1 : NULL Pointer" );
	}

	if ( sig->sig_StructID == ID_USB2_SIG )
	{
		USBPANIC( "__Task_ReallocSignal : 2 : IN    use" );
	}

	#endif

	IExec	= usbbase->usb_IExec;
	signal	= IExec->AllocSignal( -1 );

	sig->sig_Signal_Nr		= signal;
	sig->sig_Signal_Mask	= 1UL << signal;
}

// --
