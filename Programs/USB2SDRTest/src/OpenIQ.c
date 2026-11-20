
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "SDRTest.h"

// --

struct USB2_EPResource *
SDR_OpenIQ( struct USB2_Register *reg, U8 endpoint, U32 frequency, U32 requests )
{
	struct USB2_EPResource *bulk;
	struct USB2_EPResource *control;

	U32 speed;

	if ( ( USB2_Attr_Get( reg->FKT_ID, USB2Tag_Attr_Speed, &speed, sizeof( speed ) ) != sizeof( speed ) ) ||
		( speed != USBSPEED_High ) )
	{
		Printf( "I/Q capture requires a high-speed USB connection.\n" );

		return ( NULL );
	}

	bulk = USB2_EPRes_ObtainTags( reg,
		USB2Tag_EPRes_InterfaceNr, 0,
		USB2Tag_EPRes_EndPointNr, endpoint,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, requests,
		TAG_END );

	if ( ! bulk )
	{
		Printf( "Could not open the SDR Bulk IN endpoint $%02lx.\n", (U32) endpoint );

		return ( NULL );
	}

	control = reg->Res_Control;

	if ( SDR_PowerOn( control ) )
	{
		goto bailout;
	}

	if ( ! SDR_ConfigureIQ( control ) )
	{
		goto power_down;
	}

	if ( SDR_SetI2CGate( control, TRUE ) )
	{
		SDR_SetI2CGate( control, FALSE );

		goto power_down;
	}

	if ( ! SDR_R820T_Tune( control, frequency ) )
	{
		SDR_SetI2CGate( control, FALSE );

		SDR_CloseIQ( control, bulk );

		return ( NULL );
	}

	if ( SDR_SetI2CGate( control, FALSE ) )
	{
		SDR_CloseIQ( control, bulk );

		return ( NULL );
	}

	return ( bulk );

power_down:
	SDR_PowerOff( control );

bailout:
	USB2_EPRes_Release( bulk );

	return ( NULL );
}

// --
