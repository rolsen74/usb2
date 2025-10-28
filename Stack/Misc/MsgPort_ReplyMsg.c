
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

SEC_CODE void __MsgPort_ReplyMsg( struct USBBase *usbbase, PTR msg, STR file UNUSED )

#else

SEC_CODE void __MsgPort_ReplyMsg( struct USBBase *usbbase, PTR msg )

#endif

{
struct ExecIFace *IExec;

	TASK_NAME_ENTER( "__MsgPort_ReplyMsg" );

	#ifdef DO_PANIC

	if (( ! usbbase ) || ( ! msg ))
	{
		USBPANIC( "MsgPort_ReplyMsg : 3 : NULL Pointer : (%s)", file );
	}

	#endif

	IExec = usbbase->usb_IExec;
	IExec->ReplyMsg( msg );

	TASK_NAME_LEAVE();
}

// --
