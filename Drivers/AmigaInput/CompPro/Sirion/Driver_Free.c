
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

void Driver_Free( struct USB_Driver *in )
{

	// --
	if ( in->USB_IntOut_IOReq )
	{
//		if ( ps->InterruptOutIOReqUsed )
//		{
//			if ( IExec->CheckIO( (struct IORequest *)ps->InterruptOutIOReq ) == NULL )
//			{
//				IExec->AbortIO( (struct IORequest *)ps->InterruptOutIOReq );
//				IExec->WaitIO( (struct IORequest *)ps->InterruptOutIOReq );
//			}
//		}

		USBFreeRequest( in->USB_IntOut_IOReq );
		in->USB_IntOut_IOReq = NULL;
	}

	if ( in->USB_IntOut_Buffer )
	{
		FreeVec( in->USB_IntOut_Buffer );
		in->USB_IntOut_Buffer = NULL;
	}

	if ( in->USB_IntOut_MP )
	{
		FreeSysObject( ASOT_PORT, in->USB_IntOut_MP );
		in->USB_IntOut_MP = NULL;
	}




	// --
	if ( in->USB_IntIn_IOReq )
	{
//		if ( ps->InterruptOutIOReqUsed )
//		{
//			if ( IExec->CheckIO( (struct IORequest *)ps->InterruptOutIOReq ) == NULL )
//			{
//				IExec->AbortIO( (struct IORequest *)ps->InterruptOutIOReq );
//				IExec->WaitIO( (struct IORequest *)ps->InterruptOutIOReq );
//			}
//		}

		USBFreeRequest( in->USB_IntIn_IOReq );
		in->USB_IntIn_IOReq = NULL;
	}

	if ( in->USB_IntIn_Buffer )
	{
		FreeVec( in->USB_IntIn_Buffer );
		in->USB_IntIn_Buffer = NULL;
	}

	if ( in->USB_IntIn_MP )
	{
		FreeSysObject( ASOT_PORT, in->USB_IntIn_MP );
		in->USB_IntIn_MP = NULL;
	}

	// --

//	if ( ps->Interface )
//	{
//		IUSBSys->USBDeclaimInterface( ps->Interface );
//		ps->Interface = NULL;
//	}

	if ( ! in->USB_Function )
	{
		USBDeclaimFunction( in->USB_Function );
		in->USB_Function = NULL;
	}

	if ( in->Report_Buffer )
	{
		FreeVec( in->Report_Buffer );
		in->Report_Buffer = NULL;
	}
}

// --
