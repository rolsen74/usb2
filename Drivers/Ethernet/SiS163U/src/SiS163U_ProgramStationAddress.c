
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_MAC_SELECT	0x0000
#define SIS163U_MAC_DATA	0x0004
#define SIS163U_MAC_ACCESS	0x50000000
#define SIS163U_MAC_SLOT_MASK	0x0000001e
#define SIS163U_MAC_STATION_SLOT	0x00000008

// --

S32
SiS163U_ProgramStationAddress( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 selector;
	U32 data;
	U32 index;
	U32 value;
	U8 readback[6];
	U16 word;
	S32 readable;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;

	error = SiS163U_ReadRegister( control, SIS163U_MAC_SELECT, & selector );

	if ( error )
	{
		goto failed;
	}

	error = SiS163U_ReadRegister( control, SIS163U_MAC_DATA, & data );

	if ( error )
	{
		goto failed;
	}

	selector = ( selector & ~SIS163U_MAC_SLOT_MASK ) | SIS163U_MAC_ACCESS;
	data &= 0xffff0000;
	readable = TRUE;

	// Start unassociated with the BSSID slots $00, $02 and $04 cleared.
	for( index = 0; index < sizeof( unit->unit_StationAddress ); index += 2 )
	{
		error = SiS163U_WriteRegister( control, SIS163U_MAC_SELECT, selector | index );

		if ( error )
		{
			goto failed;
		}

		error = SiS163U_WriteRegister( control, SIS163U_MAC_DATA, data );

		if ( error )
		{
			goto failed;
		}
	}

	// The station address occupies slots $08, $0a and $0c.
	selector |= SIS163U_MAC_STATION_SLOT;

	for( index = 0; index < sizeof( unit->unit_StationAddress ); index += 2 )
	{
		word = (U16) unit->unit_StationAddress[index] | ( (U16) unit->unit_StationAddress[index + 1] << 8 );

		error = SiS163U_WriteRegister( control, SIS163U_MAC_SELECT, selector | index );

		if ( error )
		{
			goto failed;
		}

		error = SiS163U_WriteRegister( control, SIS163U_MAC_DATA, data | word );

		if ( error )
		{
			goto failed;
		}

		error = SiS163U_ReadRegister( control, SIS163U_MAC_DATA, & value );

		if ( error )
		{
			readable = FALSE;
		}
		else
		{
			readback[index] = value;
			readback[index + 1] = value >> 8;
		}
	}

	if ( readable )
	{
		USBINFO( "SiS163U: MAC station address readback %02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
			(U32) readback[0], (U32) readback[1], (U32) readback[2],
			(U32) readback[3], (U32) readback[4], (U32) readback[5] );
	}
	else
	{
		USBINFO( "SiS163U: station address programmed, MAC readback unavailable" );
	}

	return ( TRUE );

failed:
	USBERROR( "SiS163U: could not program the MAC station address (USB error %ld)", error );

	return ( FALSE );
}

// --
