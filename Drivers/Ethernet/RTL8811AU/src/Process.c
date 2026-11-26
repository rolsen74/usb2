
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

// --

extern const struct USB2EthOps EthClass_Ops;

// --

U32
EthClass_Entry( struct USB2DriverIFace *self UNUSED, struct USB2_DriverMessage *msg )
{
	static const struct EthClassPort empty = { 0 };

	struct USB2_EPResource *control;
	struct USB2_EPResource *rx;
	struct USB2_Register *reg;
	struct USB2IFace *usb2;
	struct EthernetUnit *unit;
	struct EthClassPort port;
	struct RTL8811AU_Board board;

	S32 registered;
	S32 running;
	S32 detached;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) ||
		( msg->VendorID != RTL8811AU_VENDOR_ID ) || ( msg->DeviceID != RTL8811AU_PRODUCT_ID ) ||
		( msg->Interface->IFC_Num != 0 ) )
	{
		return ( FALSE );
	}

	usb2 = msg->IUSB2;
	registered = FALSE;
	rx = NULL;

	CopyMem( (PTR) & empty, & port, sizeof( port ) );

	unit = AllocVecTags( sizeof( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( FALSE );
	}

	InitSemaphore( & unit->unit_CommandLock );

	NewList( & unit->unit_Openers );

	NewList( & unit->unit_Pending );

	reg = usb2->USB2_Reg_RegisterTags( USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "RTL8811AU Wi-Fi", TAG_END );

	if ( ! reg )
	{
		FreeVec( unit );

		return ( FALSE );
	}

	control = reg->Res_Control;
	unit->unit_Control = control;
	unit->unit_USB2 = usb2;

	DebugPrintF( "RTL8811AU: probing 2357:011e interface 0\n" );

	if ( ! RTL8811AU_Probe( control, unit->unit_StationAddress, & board ) )
	{
		goto done;
	}

	registered = EthClass_Register( & port, & EthClass_Ops, unit, & unit->unit_UnitNumber, USB2ETH_ADAPTER_RTL8811AU, msg );

	if ( ! registered )
	{
		goto done;
	}

	running = RTL8811AU_PowerOn( control );

	if ( ! running )
	{
		DebugPrintF( "RTL8811AU: MAC power-on failed\n" );
	}
	else
	{
		running = RTL8811AU_LoadFirmware( control );

		if ( running )
		{
			DebugPrintF( "RTL8811AU: firmware running\n" );

			rx = usb2->USB2_EPRes_ObtainTags( reg,
				USB2Tag_EPRes_InterfaceNr, 0,
				USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
				USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
				USB2Tag_EPRes_BufferSize, 16384,
				USB2Tag_EPRes_NrOfIORequest, WIRELESS_RX_REQUEST_COUNT,
				USB2Tag_EPRes_AllowShortPackets, TRUE,
				TAG_END );

			running = FALSE;

			if ( ( rx ) && ( rx->EndPointMax ) && ( rx->IORequests ) && ( rx->IORequests[0] ) &&
				( rx->Buffers ) && ( rx->Buffers[0] ) )
			{
				unit->unit_Tx = usb2->USB2_EPRes_ObtainTags( reg,
					USB2Tag_EPRes_InterfaceNr, 0,
					USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
					USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
					USB2Tag_EPRes_BufferSize, RTL8811AU_TX_BUFFER_SIZE,
					TAG_END );

				if ( ( unit->unit_Tx ) && ( ( ! unit->unit_Tx->EndPointMax ) || ( ! unit->unit_Tx->IORequests ) ||
					( ! unit->unit_Tx->IORequests[0] ) || ( ! unit->unit_Tx->Buffers ) || ( ! unit->unit_Tx->Buffers[0] ) ) )
				{
					usb2->USB2_EPRes_Release( unit->unit_Tx );

					unit->unit_Tx = NULL;
				}

				running = RTL8811AU_StartRadio( unit, & board, rx->EndPointMax );

				if ( ! running )
				{
					DebugPrintF( "RTL8811AU: radio setup failed\n" );
				}
			}
			else
			{
				DebugPrintF( "RTL8811AU: could not allocate bulk receive resources\n" );
			}
		}
		else
		{
			DebugPrintF( "RTL8811AU: firmware startup failed\n" );
		}
	}

	if ( ! running )
	{
		RTL8811AU_PowerOff( control );

		ObtainSemaphore( & unit->unit_CommandLock );

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

		ReleaseSemaphore( & unit->unit_CommandLock );
	}

	detached = Process_Main( unit, reg, running ? rx : NULL, & board );

	ObtainSemaphore( & unit->unit_CommandLock );

	unit->unit_Detached = TRUE;

	RTL8811AU_SetOnline( unit, FALSE );

	memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

	Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, S2ERR_OUTOFSERVICE, S2WERR_UNIT_DISCONNECTED );

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( ( running ) && ( ! detached ) )
	{
		RTL8811AU_PowerOff( control );
	}

done:

	if ( unit->unit_Tx )
	{
		usb2->USB2_EPRes_Release( unit->unit_Tx );
	}

	if ( rx )
	{
		usb2->USB2_EPRes_Release( rx );
	}

	usb2->USB2_Reg_Unregister( reg );

	if ( registered )
	{
		EthClass_Unregister( & port );
	}
	else
	{
		FreeVec( unit );
	}

	return ( registered );
}

// --
