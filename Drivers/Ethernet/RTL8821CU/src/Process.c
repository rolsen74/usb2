
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "EthClass.h"
#include "Radio.h"
#include "WiFiCommand.h"
#include <devices/timer.h>
#include <interfaces/timer.h>
#include <usb2wifi.h>

// --

S32
RTL8821CU_AdapterPresent ( struct USB2IFace * usb2, USB2_ID id )
{
	struct USB2_Function copy;

	return ( usb2->USB2_Fkt_FindTags ( USB2Tag_Find_VendorID, RTL8821CU_VENDOR_ID, USB2Tag_Find_DeviceID, RTL8821CU_PRODUCT_ID,
									   USB2Tag_Find_FunctionID, &id, USB2Tag_Find_Copy, &copy, TAG_END )
			 == &copy );
}

// --

// Called with the command semaphore held.
void
RTL8821CU_Detach ( struct EthernetUnit * unit )
{
	if ( unit->unit_Detached )
	{
		return;
	}

	unit->unit_Detached = TRUE;
	unit->unit_Handshake = FALSE;
	unit->unit_EAPOLLength = 0;

	RTL8821CU_SetOnline ( unit, FALSE );

	memset ( unit->unit_Keys, 0, sizeof ( unit->unit_Keys ) );

	Sana2_SignalEvent ( &unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

	Sana2_AbortPending ( &unit->unit_Pending, NULL, NULL, S2ERR_OUTOFSERVICE, S2WERR_UNIT_DISCONNECTED );
}

// --

static void
Process_Main ( struct EthernetUnit * unit, struct USB2IFace * usb2 )
{
	struct USB2_NotifyMessage * notice;
	struct USB2_Register *		reg;

	S32 detached;
	S32 start;

	reg		 = unit->unit_USB_Register;
	detached = FALSE;

	for ( ;; )
	{
		while ( ( notice = (PTR)GetMsg ( reg->Stack_MsgPort ) ) )
		{
			if ( notice->nm_Command == USBNC_Detached )
			{
				detached = TRUE;
			}

			ReplyMsg ( (PTR)notice );
		}

		// USB2 can omit detach notifications while a registration holds the function.
		if ( ( detached ) || ( ! RTL8821CU_AdapterPresent ( usb2, reg->FKT_ID ) ) )
		{
			ObtainSemaphore ( &unit->unit_CommandLock );

			RTL8821CU_Detach ( unit );

			ReleaseSemaphore ( &unit->unit_CommandLock );

			return;
		}

		if ( SetSignal ( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			// Hub removal marks the function detached before signalling this task.
			if ( ! RTL8821CU_AdapterPresent ( usb2, reg->FKT_ID ) )
			{
				ObtainSemaphore ( &unit->unit_CommandLock );

				RTL8821CU_Detach ( unit );

				ReleaseSemaphore ( &unit->unit_CommandLock );
			}

			return;
		}

		ObtainSemaphore ( &unit->unit_CommandLock );

		Process_PacketIO ( unit, usb2 );

		if ( unit->unit_Detached )
		{
			ReleaseSemaphore ( &unit->unit_CommandLock );

			return;
		}

		RTL8821CU_CheckLink ( unit );

		start = unit->unit_StartAuto && unit->unit_RadioReady && ! unit->unit_AssocID;

		if ( start )
		{
			unit->unit_StartAuto = FALSE;
		}

		ReleaseSemaphore ( &unit->unit_CommandLock );

		if ( start )
		{
			Wireless_StartAuto( USB2WIFI_RTL8821CU, unit->unit_UnitNr );
		}

		Delay ( 1 );
	}
}

// --

extern const struct USB2EthOps EthClass_Ops;

// --

U32
EthClass_Entry ( struct USB2DriverIFace * Self UNUSED, struct USB2_DriverMessage * msg )
{
	struct EthernetUnit * unit;
	struct EthClassPort	  port;
	struct TimeRequest	  timer;

	S32 registered;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( msg->Interface->IFC_Num != RTL8821CU_INTERFACE ) )
	{
		return ( FALSE );
	}

	unit = AllocVecTags ( sizeof ( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( FALSE );
	}

	memset ( &timer, 0, sizeof ( timer ) );

	timer.Request.io_Message.mn_Length = sizeof ( timer );

	if ( OpenDevice ( TIMERNAME, UNIT_MICROHZ, &timer.Request, 0 ) )
	{
		DebugPrintF ( "RTL8821CU: could not open timer.device\n" );

		FreeVec ( unit );

		return ( FALSE );
	}

	unit->unit_Timer = (PTR)GetInterface ( (PTR)timer.Request.io_Device, "main", 1, NULL );

	if ( ! unit->unit_Timer )
	{
		CloseDevice ( &timer.Request );

		FreeVec ( unit );

		return ( FALSE );
	}

	InitSemaphore ( &unit->unit_CommandLock );

	NewList ( &unit->unit_Openers );

	NewList ( &unit->unit_Pending );

	port.Request   = (struct IORequest){ 0 };
	port.Interface = NULL;
	port.Unit	   = NULL;
	registered	   = FALSE;

	if ( Process_Setup_Device ( unit, msg ) )
	{
		registered = EthClass_Register ( &port, &EthClass_Ops, unit, &unit->unit_UnitNr, USB2ETH_ADAPTER_RTL8821CU, msg );

		if ( registered )
		{
			Process_Main ( unit, msg->IUSB2 );
		}
	}

	Process_Free ( unit, msg->IUSB2 );

	DropInterface ( (PTR)unit->unit_Timer );

	CloseDevice ( &timer.Request );

	if ( registered )
	{
		EthClass_Unregister ( &port );
	}
	else
	{
		FreeVec ( unit );
	}

	return ( registered );
}

// --
