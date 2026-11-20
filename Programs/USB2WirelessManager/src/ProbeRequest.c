
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

U32
Wireless_ProbeRequest( struct Wireless *wireless, U8 *frame )
{
	static const U8 rates[] = { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 };

	U32 length;

	Wireless_Frame_Header( wireless, frame, 0x40 );

	memset( frame + 4, 0xff, WIRELESS_ADDR_SIZE );

	memset( frame + 16, 0xff, WIRELESS_ADDR_SIZE );

	frame[24] = 0; // SSID information element
	frame[25] = wireless->SSIDLength;

	memcpy( frame + 26, wireless->SSID, wireless->SSIDLength );

	length = 26 + wireless->SSIDLength;
	frame[length++] = 1; // Supported rates
	frame[length++] = wireless->ScanRT2870 ? 8 : 4;

	memcpy( frame + length, rates, wireless->ScanRT2870 ? 8 : 4 );

	length += wireless->ScanRT2870 ? 8 : 4;

	if ( wireless->ScanRT2870 )
	{
		frame[length++] = 50; // Extended supported rates
		frame[length++] = 4;

		memcpy( frame + length, rates + 8, 4 );

		length += 4;
	}

	return ( length );
}

// --
