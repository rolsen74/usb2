
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_RX_FILTER	0xc04b6da0

// --

void
SiS163U_StartRXCheck( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 value;
	U32 macMode;
	U32 filter;
	U32 usbControl;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;

	if ( ( error = SiS163U_ReadRegister( control, 0x00f8, & value ) ) ||
		( error = SiS163U_WriteRegister( control, 0x00f8, value | 0x00004000 ) ) ||
		( error = SiS163U_ReadRegister( control, 0x00f8, & value ) ) ||
		( error = SiS163U_WriteRegister( control, 0x00f8, ( value & 0x00007fff ) | 0x030d8000 ) ) ||
		( error = SiS163U_ReadRegister( control, 0x0000, & value ) ) ||
		( error = SiS163U_WriteRegister( control, 0x0000, value | 0x54000000 ) ) ||
		( error = SiS163U_WriteRegister( control, 0x0054, 1 ) ) ||
		( error = SiS163U_WriteRegister( control, 0x0000, SIS163U_MAC_RX_MODE ) ) ||
		( error = SiS163U_ReadRegister( control, 0x009c, & value ) ) ||
		( error = SiS163U_WriteRegister( control, 0x009c, value | 0x00010000 ) ) ||
		( error = SiS163U_ReadRegister( control, 0x0070, & value ) ) ||
		( error = SiS163U_WriteRegister( control, 0x0070, value | 0x04000000 ) ) )
	{
		USBERROR( "SiS163U: receive check setup failed, USB error %ld", error );

		return;
	}

	SiS163U_LogRXState( unit, "before bulk IN" );

	SiS163U_SubmitRXCheck( unit );

	error = SiS163U_WriteRegister( control, 0x0014, SIS163U_RX_FILTER );

	if ( error )
	{
		SiS163U_StopRX( unit );

		USBERROR( "SiS163U: could not enable the receive filter, USB error %ld", error );

		return;
	}

	unit->unit_RxReady = TRUE;

	if ( ( error = SiS163U_ReadRegister( control, 0x0000, & macMode ) ) ||
		( error = SiS163U_ReadRegister( control, 0x0014, & filter ) ) ||
		( error = SiS163U_ReadRegister( control, 0x00f8, & usbControl ) ) )
	{
		USBERROR( "SiS163U: could not read back receive registers, USB error %ld", error );
	}
	else
	{
		USBINFO( "SiS163U: receive registers $0000 = $%08lx, $0014 = $%08lx (wrote $%08lx), $00F8 = $%08lx",
			macMode, filter, (U32) SIS163U_RX_FILTER, usbControl );
	}

	USBINFO( "SiS163U: passive receive check started on channels 1 through %lu, %lu-byte bulk IN requests",
		(U32) SIS163U_CHANNEL_MAX, unit->unit_Bulk_Rx_Resource->BufferSize );
}

// --
