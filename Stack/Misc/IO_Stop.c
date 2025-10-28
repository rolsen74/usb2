
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

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void __IO_Stop( struct USBBase *usbbase, PTR ioreq, STR file UNUSED )

#else

SEC_CODE void __IO_Stop( struct USBBase *usbbase, PTR ioreq )

#endif

{
struct ExecIFace *IExec;

	TASK_NAME_ENTER( "__IO_Stop" );

	#ifdef DO_PANIC

	if (( ! usbbase ) || ( ! ioreq ))
	{
		USBPANIC( "__IO_Stop : NULL Pointer : (%s)", file );
	}

	if ( IS_USB_IOREQ(ioreq) ) 
	{
		IOREQUEST_VALID(ioreq);

		if ( usbbase->usb_LockSemaphore.us_Semaphore.ss_Owner == usbbase->usb_IExec->FindTask(NULL) )
		{
			USBPANIC( "__IO_Stop : You must NOT hold usb_LockSemaphore for this call : (%s)", (file)?file:"<NULL>" );
		}
	}

	#endif

	IExec = usbbase->usb_IExec;

	if ( IExec->CheckIO(ioreq) == NULL )
	{	
		IExec->AbortIO(ioreq);
	}	
	else
	{
		USBINFO( "__IO_Stop : Not acitve?? : (%s)", file );
	}

	IExec->WaitIO(ioreq);

	TASK_NAME_LEAVE();
}

// --
