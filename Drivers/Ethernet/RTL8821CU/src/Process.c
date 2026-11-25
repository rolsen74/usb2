
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
#include <usb2wifi.h>

// --

static S32
AdapterPresent ( struct USB2IFace * usb2, USB2_ID id )
{
	struct USB2_Function copy;

	return ( usb2->USB2_Fkt_FindTags ( USB2Tag_Find_VendorID, RTL8821CU_VENDOR_ID, USB2Tag_Find_DeviceID, RTL8821CU_PRODUCT_ID,
									   USB2Tag_Find_FunctionID, &id, USB2Tag_Find_Copy, &copy, TAG_END )
			 == &copy );
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

	while ( ! ( SetSignal ( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) )
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
		if ( ( detached ) || ( ! AdapterPresent ( usb2, reg->FKT_ID ) ) )
		{
			ObtainSemaphore ( &unit->unit_CommandLock );

			unit->unit_Detached = TRUE;
			RTL8821CU_SetOnline( unit, FALSE );

			memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

			Sana2_SignalEvent ( &unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

			Sana2_AbortPending ( &unit->unit_Pending, NULL, NULL, S2ERR_OUTOFSERVICE, S2WERR_UNIT_DISCONNECTED );

			ReleaseSemaphore ( &unit->unit_CommandLock );

			return;
		}

		ObtainSemaphore ( &unit->unit_CommandLock );

		Process_PacketIO ( unit, usb2 );

		RTL8821CU_CheckLink ( unit );

		start = unit->unit_StartAuto && unit->unit_RadioReady && unit->unit_TxReady && ! unit->unit_AssocID;
		unit->unit_StartAuto = FALSE;

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
