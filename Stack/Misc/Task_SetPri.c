
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

SEC_CODE S32 __Task_SetPri( struct USBBase *usbbase, S32 pri )
{
struct ExecIFace *IExec;
struct Task *Self;
S32 oldpri;

	IExec	= usbbase->usb_IExec;
	Self	= IExec->FindTask( NULL );
	oldpri	= IExec->SetTaskPri( Self, pri );

	return( oldpri );
}

// --

