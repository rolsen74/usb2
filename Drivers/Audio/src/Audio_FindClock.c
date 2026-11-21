
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Audio_FindClock( struct AudioUnit *unit, U32 config_size, U8 terminal, U8 endpoint )
{
	const U8 *data;
	const U8 *limit;
	const U8 *control;
	const U8 *associated;
	const U8 *entity;

	U32 control_count;
	U32 first_interface;
	U8 clock;
	U8 terminal_type;

	data = (PTR) unit->StartMsg->ConfigDescriptors;
	limit = data + config_size;
	control = NULL;
	associated = NULL;
	control_count = 0;
	first_interface = 256;

	while( data < limit )
	{
		if ( ( data[0] < 2 ) || ( data[0] > limit - data ) )
		{
			return ( FALSE );
		}

		if ( ( data[1] == DSCTYPE_Interface_Association ) && ( data[0] >= 8 ) &&
			( unit->InterfaceNr >= data[2] ) && ( unit->InterfaceNr < data[2] + data[3] ) )
		{
			first_interface = data[2];
		}
		else if ( ( data[1] == DSCTYPE_Interface ) && ( data[0] >= 9 ) &&
			( ! data[3] ) && ( data[5] == USBCLASS_Audio ) && ( data[6] == 1 ) &&
			( data[7] == AUDIO_PROTOCOL_UAC2 ) )
		{
			control = data;
			control_count++;

			if ( data[2] == first_interface )
			{
				associated = data;
			}
		}

		data += data[0];
	}

	if ( first_interface < 256 )
	{
		control = associated;
	}
	else if ( control_count != 1 )
	{
		control = NULL;
	}

	if ( ( ! control ) || ( ! terminal ) )
	{
		return ( FALSE );
	}

	// The stream links to a USB input terminal for playback, or output for capture.
	terminal_type = ( endpoint & EPADR_Dir_Mask ) ? 3 : 2;
	clock = 0;
	entity = NULL;

	for( data = control + control[0]; ( data < limit ) && ( data[1] != DSCTYPE_Interface ); data += data[0] )
	{
		if ( ( data[0] >= 12 ) && ( data[1] == 0x24 ) && ( data[2] == terminal_type ) &&
			( data[3] == terminal ) && ( data[4] == 1 ) && ( data[5] == 1 ) )
		{
			clock = data[terminal_type == 3 ? 8 : 7];
			break;
		}
	}

	if ( ! clock )
	{
		return ( FALSE );
	}

	for( data = control + control[0]; ( data < limit ) && ( data[1] != DSCTYPE_Interface ); data += data[0] )
	{
		if ( ( data[0] >= 8 ) && ( data[1] == 0x24 ) && ( data[2] == 0x0a ) &&
			( data[3] == clock ) && ( data[5] & 1 ) )
		{
			entity = data;
			break;
		}
	}

	if ( ! entity )
	{
		return ( FALSE );
	}

	unit->ClockInterface = control[2];
	unit->ClockEntity = clock;

	return ( TRUE );
}

// --
