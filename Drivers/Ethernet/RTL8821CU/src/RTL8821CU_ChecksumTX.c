
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
RTL8821CU_ChecksumTX( U8 *descriptor )
{
	U32 index;
	U16 checksum;

	descriptor[28] = 0;
	descriptor[29] = 0;
	checksum = 0;

	for( index = 0; index < 32; index += 2 )
	{
		checksum ^= (U16) descriptor[index] | ( (U16) descriptor[index + 1] << 8 );
	}

	descriptor[28] = checksum;
	descriptor[29] = checksum >> 8;
}

// --
