
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "usb2_rev.h"

// --

#if 1

static void Full_Boot ( void );
static void Full_Boot_Entry ( void );

// --
// Note: don't add Static

SEC_RODATA const struct Resident fullboot_res = { /* rt_MatchWord	*/ RTC_MATCHWORD,
												  /* rt_MatchTag	*/ (PTR)( &fullboot_res + 0 ),
												  /* rt_EndSkip	*/ (PTR)( &fullboot_res + 1 ),
												  /* rt_Flags		*/ RTF_NATIVE | RTF_AFTERDOS,
												  /* rt_Version	*/ 1,
												  /* rt_Type		*/ NT_UNKNOWN,
												  /* rt_Pri		*/ -49,
												  /* rt_Name		*/ ( STR ) "Universal Serial Bus - Fullboot",
												  /* rt_IdString	*/ ( STR ) "Universal Serial Bus - Fullboot",
												  /* rt_Init		*/ (PTR)Full_Boot };

// --

SEC_CODE static void
Full_Boot_Failed ( struct ExecIFace * IExec )
{
	struct ExecBase * SysBase;
	struct USBBase *  usbbase;

	SysBase = (PTR)IExec->Data.LibBase;

	usbbase = (PTR)IExec->FindName ( &SysBase->DeviceList, "usb2.device" );

	if ( usbbase )
	{
		USB2_FullBoot_Complete ( usbbase, FALSE );
	}
}

// --

SEC_CODE static void
Full_Boot ( void )
{
	struct ExecIFace * IExec;
	struct DOSIFace *  IDOS;
	struct Library *   DOSBase;
	struct Process *   process;

	IExec	= USB2_GetExecIFace ();
	process = NULL;

	IExec->DebugPrintF ( "USB : Full Boot (" VERS ")\n" );

#ifdef MSD_DEBUG
	IExec->DebugPrintF ( "USB : MSD debug active\n" );
#endif

#ifdef USB2_BIND_DEBUG
	IExec->DebugPrintF ( "USB : class binding debug active\n" );
#endif

#ifdef SIS163U_RX_TRACE
	IExec->DebugPrintF ( "USB : SiS163U EHCI RX trace active\n" );
#endif

	IDOS = NULL;

	DOSBase = IExec->OpenLibrary ( "dos.library", 50 );

	if ( ! DOSBase )
	{
		//		USBERROR( "DOS not around yet, waiting for DOS notify" );
		goto bailout;
	}

	IDOS = (PTR)IExec->GetInterface ( DOSBase, "main", 1, NULL );

	if ( ! IDOS )
	{
		//		USBERROR( "DOS not around yet, waiting for DOS notify" );
		goto bailout;
	}

	process = IDOS->CreateNewProcTags ( NP_Name, "Universal Serial Bus - Fullboot Process", NP_Child, FALSE, NP_Priority, 0,
										NP_Entry, Full_Boot_Entry, NP_UserData, IDOS, TAG_END );

bailout:

	if ( ! process )
	{
		Full_Boot_Failed ( IExec );
	}

	if ( ( ! process ) && ( IDOS ) )
	{
		IExec->DropInterface ( (PTR)IDOS );
	}

	if ( ( ! process ) && ( DOSBase ) )
	{
		IExec->CloseLibrary ( DOSBase );
	}
}

// --

SEC_CODE static void
Full_Boot_Entry ( void )
{
	struct TimeRequest * timerreq;
	struct Library *	 DOSBase;
	struct IORequest *	 usbreq;
	struct ExecIFace *	 IExec;
	struct DOSIFace *	 IDOS;
	struct MsgPort *	 timerport;
	struct MsgPort *	 usbport;
	struct USBBase *	 usbbase;
	S32					 timeout;
	U32					 lock1;
	U32					 lock2;
	U32					 lock3;
	U8					 timerstat;
	U8					 usbstat;
	S32					 signalled;

	// ----

	timerstat = 1;
	timerreq  = NULL;
	timerport = NULL;
	usbstat	  = 1;
	usbreq	  = NULL;
	usbport	  = NULL;
	signalled = FALSE;

	// ----

	IExec	= USB2_GetExecIFace ();
	IDOS	= (PTR)( (struct Task *)IExec->FindTask ( NULL ) )->tc_UserData;
	DOSBase = IDOS->Data.LibBase;

	// Prevent DOS opening any windows
	IDOS->SetProcWindow ( (PTR)-1 );

	// ----

	usbport = IExec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! usbport )
	{
		//		USBERROR( "Error creating USB IOPort" );
		goto bailout;
	}

	usbreq = IExec->AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( struct IORequest ), ASOIOR_ReplyPort, usbport,
										 TAG_END );

	if ( ! usbreq )
	{
		//		USBERROR( "Error creating USB IORequest" );
		goto bailout;
	}

	usbstat = IExec->OpenDevice ( "usb2.device", 0, usbreq, 0 );

	// OpenDevice changes DebugLevel so reset it

	if ( usbstat )
	{
		//		USBERROR( "Error Opening usb.device" );
		goto bailout;
	}

	// ----

	timerport = IExec->AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! timerport )
	{
		//		USBERROR( "Error creating Timer IOPort" );
		goto bailout;
	}

	timerreq = IExec->AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( struct TimeRequest ), ASOIOR_ReplyPort,
										   timerport, TAG_END );

	if ( ! timerreq )
	{
		//		USBERROR( "Error creating USB IORequest" );
		goto bailout;
	}

	timerstat = IExec->OpenDevice ( "timer.device", UNIT_VBLANK, (PTR)timerreq, 0 );

	if ( timerstat )
	{
		//		USBERROR( "Error Opening timer.device" );
		goto bailout;
	}

	// ----

	timeout = 300;

	lock1 = 0;
	lock2 = 0;
	lock3 = 0;

	while ( timeout )
	{
		// Looks like ENV: RAM: T: is mounted in that sequence

		if ( ! lock1 )
		{
			lock1 = IDOS->Lock ( "t:", ACCESS_READ );
		}

		if ( lock1 )
		{
			if ( ! lock2 )
			{
				lock2 = IDOS->Lock ( "ram:", ACCESS_READ );
			}

			if ( lock2 )
			{
				if ( ! lock3 )
				{
					lock3 = IDOS->Lock ( "env:", ACCESS_READ );
				}

				if ( lock3 )
				{
					break;
				}
			}
		}

		// ** Wait a while
		timerreq->Time.Seconds		 = 0;
		timerreq->Time.Microseconds	 = 250000;
		timerreq->Request.io_Command = TR_ADDREQUEST;
		IExec->DoIO ( (PTR)timerreq );

		timeout--;
	}

	if ( lock1 )
	{
		IDOS->UnLock ( lock1 );
	}

	if ( lock2 )
	{
		IDOS->UnLock ( lock2 );
	}

	if ( lock3 )
	{
		IDOS->UnLock ( lock3 );
	}

	if ( usbreq )
	{
		usbbase = (PTR)usbreq->io_Device;

		if ( usbbase )
		{
			if ( usbbase->usb_Master_Task.tn_TaskAdr )
			{
				IExec->Signal ( usbbase->usb_Master_Task.tn_TaskAdr, SIGBREAKF_CTRL_E );

				signalled = TRUE;
			}

			SEMAPHORE_OBTAIN ( &usbbase->usb_MSDSemaphore );

			if ( ! usbbase->usb_MounterBase )
			{
				usbbase->usb_MounterBase = IExec->OpenLibrary ( "mounter.library", 0 );
			}

			if ( ( usbbase->usb_MounterBase ) && ( ! usbbase->usb_IMounterPriv ) )
			{
				usbbase->usb_IMounterPriv = (PTR)IExec->GetInterface ( (PTR)usbbase->usb_MounterBase, "private", 1, NULL );
			}

			SEMAPHORE_RELEASE ( &usbbase->usb_MSDSemaphore );

			if ( usbbase->usb_IMounterPriv )
			{
				//				IExec->DebugPrintF( "\nCalling Mount FullBoot\n\n" );

				usbbase->usb_IMounterPriv->FullBooted ();
			}
			else
			{
				IExec->DebugPrintF ( "\nFailed to open Mounter Private Interface\n\n" );
			}
		}
		else
		{
			IExec->DebugPrintF ( "\nusbbase NULL Pointer\n\n" );
		}
	}
	else
	{
		IExec->DebugPrintF ( "\nusbreq NULL Pointer\n\n" );
	}

bailout:

	if ( ! signalled )
	{
		Full_Boot_Failed ( IExec );
	}

	// ----

	if ( ! timerstat )
	{
		IExec->CloseDevice ( (PTR)timerreq );
	}

	if ( timerreq )
	{
		IExec->FreeSysObject ( ASOT_IOREQUEST, timerreq );
	}

	if ( timerport )
	{
		IExec->FreeSysObject ( ASOT_PORT, timerport );
	}

	// ----

	if ( ! usbstat )
	{
		IExec->CloseDevice ( (PTR)usbreq );
	}

	if ( usbreq )
	{
		IExec->FreeSysObject ( ASOT_IOREQUEST, usbreq );
	}

	if ( usbport )
	{
		IExec->FreeSysObject ( ASOT_PORT, usbport );
	}

	IExec->DropInterface ( (PTR)IDOS );

	IExec->CloseLibrary ( DOSBase );

	// ----
}

// --

#endif
