
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "SDRTest.h"

// --

U8
SDR_FindBulkIn ( const U8 * config, U32 size, U32 interface )
{
	U32 length;
	U8	address;
	S32 active;

	address = 0;
	active	= FALSE;

	if ( ( size < sizeof ( struct USB2_Config_Desc ) ) || ( config[0] < sizeof ( struct USB2_Config_Desc ) )
		 || ( config[1] != DSCTYPE_Configuration ) || ( ( (U32)config[2] | ( (U32)config[3] << 8 ) ) != size ) )
	{
		return ( 0 );
	}

	while ( size )
	{
		length = config[0];

		if ( ( size < 2 ) || ( length < 2 ) || ( length > size ) )
		{
			return ( 0 );
		}

		if ( config[1] == DSCTYPE_Interface )
		{
			active = ( length >= sizeof ( struct USB2_Interface_Desc ) ) && ( config[2] == interface ) && ( config[3] == 0 );
		}
		else if ( ( active ) && ( config[1] == DSCTYPE_EndPoint ) && ( length >= 7 ) && ( config[2] & EPADR_Dir_Mask )
				  && ( EPADR_Dir_Number ( config[2] ) ) && ( ( config[3] & EPATT_Type_Mask ) == EPATT_Type_Bulk ) )
		{
			address = config[2];
			break;
		}

		config += length;
		size -= length;
	}

	return ( address );
}

// --
