
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

S32 Driver_Function( struct USBFDIFace *Self, struct USBFDStartupMsg *msg, U32 type UNUSED )
{
struct USB_Driver in;
struct libBase *libBase;
S32 retval;
S32 stat;

	retval = USBERR_NOMEM;

	libBase = (PTR) Self->Data.LibBase;

	// -- Init

	memset( & in, 0, sizeof( in ));

	in.libBase				= libBase;
	in.USB_IOReq			= msg->USBReq;
	in.USB_RawFunction		= msg->Object;
	in.USBSysBase			= (PTR) msg->USBReq->io_Device;

	// --

	stat = Driver_Init( & in );

	if ( stat )
	{
		stat = Driver_AI_Add( & in );

		if ( stat )
		{
			Driver_Main( & in );

			retval = USBERR_NOERROR;
		}

		Driver_AI_Free( & in );
	}

	Driver_Free( & in );

	return( retval );

	#if 0
struct ExecIFace *IExec;
struct XBoxBase *libBase;
struct xbox *xb;
int32 retval;

	libBase = (struct XBoxBase *)Self->Data.LibBase;

	IExec = libBase->lib_IExec;

	#ifdef DEBUG
	IExec->DebugPrintF( "[XBox] Function\n" );
	#endif

	retval = USBERR_NOMEM;

    /* Allocate an internal structure, so we can be called many times and still use
     * same code without interferrence from other mice that maybe be connected */

	xb = IExec->AllocVec( sizeof(struct xbox), MEMF_CLEAR|MEMF_SHARED );

	if ( xb )
	{
		xb->IAIN = libBase->lib_IAIN;
		xb->IExec = IExec;
		xb->USBReq = msg->USBReq;
		xb->libBase = libBase;
		xb->UsbRawFunction = msg->Object;

		if ( XBoxStartup( xb ))
		{
			XBoxMainLoop( xb );

			retval = USBERR_NOERROR;
		}

		XBoxShutdown( xb );

		IExec->FreeVec( xb );
	}
	#endif

	return( retval );
}
