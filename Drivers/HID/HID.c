
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_RODATA const struct KeyboardMask KeyboardData[] =
{
{	USBHID_BOOTKBD_MODF_LSHIFT,		IEQUALIFIER_LSHIFT,		0x60,	0 },
{	USBHID_BOOTKBD_MODF_RSHIFT,		IEQUALIFIER_RSHIFT,		0x61,	0 },
{	USBHID_BOOTKBD_MODF_LCTRL |
	USBHID_BOOTKBD_MODF_RCTRL,		IEQUALIFIER_CONTROL,	0x63, 	0 },
{	USBHID_BOOTKBD_MODF_LALT,		IEQUALIFIER_LALT,		0x64,	0 },
{	USBHID_BOOTKBD_MODF_RALT,		IEQUALIFIER_RALT,		0x65,	0 },
{	USBHID_BOOTKBD_MODF_LCOMMAND,	IEQUALIFIER_LCOMMAND,	0x66,	0 },
{	USBHID_BOOTKBD_MODF_RCOMMAND,	IEQUALIFIER_RCOMMAND,	0x67,	0 },
{	0,								0,						0,		0 },
};

#define USB2_KEYMAP_STORAGE SEC_RODATA
#include "KeyboardMap.h"
#undef USB2_KEYMAP_STORAGE

// --

SEC_CODE U32 HID_Entry( struct USBBase *usbbase, struct USB2_DriverMessage *msg )
{
struct USB2_IORequest *ioreq;
struct HIDData *hd;
U32 retval;

	// --

	TASK_NAME_ENTER( "HID_Entry" );

	struct RealFunctionNode *fn UNUSED = (PTR) msg->Function;

	#ifdef DO_DEBUG
	U32 startadr = fn->fkt_Address;
	usbbase->usb_IExec->DebugPrintF( "\n##\n## HID_Entry : Adr #%lu : Enter\n##\n\n", fn->fkt_Address );
	#endif

	// --

	retval = TASK_Return_Error;

	#ifdef DO_INFO
	struct ExecIFace *IExec = usbbase->usb_IExec;
	IExec->Disable();
	IExec->DebugPrintF( "\nStartup new HID\n" );
	IExec->DebugPrintF( "HCD Index   : %ld\n", fn->fkt_HCD->hn_HCDIndex );
	IExec->DebugPrintF( "Tier        : %ld\n", fn->fkt_Tier );
	IExec->DebugPrintF( "Addr Nr     : %ld\n", fn->fkt_Address );
	IExec->DebugPrintF( "Port Nr     : %ld\n\n", fn->fkt_PortNr );
	IExec->Enable();
	#endif

	// --

	hd = MEM_ALLOCVEC( sizeof( struct HIDData ), TRUE );

	if ( hd )
	{
		#ifdef DO_DEBUG
		hd->StructID = ID_IN_HID;
		#endif
		hd->StartMsg = msg;

		if ( HID_Init( usbbase, hd ))
		{
			// --
			// Just start to Read from Interrupts

			for( int cnt=0 ; cnt<HID_IOReqCount ; cnt++ )
			{
				ioreq = hd->Res_Interrupt->IORequests[cnt];
//				ioreq->io_Data		= hd->Res_Interrupt->Buffers[cnt];
//				ioreq->io_Length	= hd->Res_Interrupt->BufferSize;
//				ioreq->io_Command	= CMD_READ;
				IO_SEND( ioreq );
			}

			// --

			/**/ if ( hd->Driver_Mode == HID_DMode_Boot )
			{
				HID_Main_Boot( usbbase, hd );
			}
			else if ( hd->Driver_Mode == HID_DMode_Report )
			{
				HID_Main_Report( usbbase, hd );
			}
			else
			{
				USBDEBUG( "HID_Entry : Invalid Mode %ld", hd->Driver_Mode );
			}
		}
		else
		{
			USBDEBUG( "HID_Entry : Error init failed" );
		}

		HID_Free( usbbase, hd );
		MEM_FREEVEC( hd );
	}
	else
	{
		USBDEBUG( "HID_Entry : Error allocating memory" );
	}

	// --

	#ifdef DO_DEBUG
	usbbase->usb_IExec->DebugPrintF( "\n##\n## HID_Entry : Adr #%lu (%lu) : FN    %p : Locks %2ld : Leave\n##\n\n", fn->fkt_Address, startadr, fn, fn->fkt_Locks );
	#endif

	TASK_NAME_LEAVE();

	return( retval );
}

// --
