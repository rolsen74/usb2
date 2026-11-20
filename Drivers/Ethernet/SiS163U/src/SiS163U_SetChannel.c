
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
SiS163U_SetChannel( struct EthernetUnit *unit, U32 channel )
{
	struct USB2_EPResource *control;

	U16 filter;
	S32 error;

	if ( ( channel < 1 ) || ( channel > SIS163U_CHANNEL_MAX ) )
	{
		return ( USB2Err_Stack_InvalidStructure );
	}

	if ( channel == unit->unit_Channel )
	{
		return ( 0 );
	}

	control = unit->unit_USB_Register->Res_Control;
	filter = ( ( channel == 5 ) || ( channel == 13 ) ) ? 0x0c34 : 0x0c30;
	unit->unit_Channel = 0;

	error = SiS163U_WriteRF( control, 0x07, 0 );

	if ( error )
	{
		return ( error );
	}

	error = SiS163U_WriteRF( control, 0x07, 0x1000 | ( channel - 1 ) );

	if ( error )
	{
		return ( error );
	}

	error = SiS163U_WriteRF( control, 0x13, filter );

	if ( error )
	{
		return ( error );
	}

	error = SiS163U_WriteRF( control, 0x13, 0x4c30 );

	if ( error )
	{
		return ( error );
	}

	unit->unit_Channel = channel;

	USBINFO( "SiS163U: RF type $08 tuned to channel %lu", channel );

	return ( 0 );
}

// --
