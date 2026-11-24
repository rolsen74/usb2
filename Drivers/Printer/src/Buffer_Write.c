
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
Buffer_Write( struct PrinterUnit *unit, U8 *data, U32 size )
{
	struct PrtBuffer *pb;

	U32 accepted;
	U32 len;

	accepted = 0;

	while ( size )
	{
		pb = unit->unit_Buffer_Active;

		if ( ! pb )
		{
			pb = (PTR)RemHead( unit->unit_Buffer_FreeList );

			if ( ! pb )
			{
				if ( unit->unit_Buffer_Allocated == PRINTER_BUFFER_COUNT )
				{
					break;
				}

				pb = AllocVecTags( sizeof( *pb ) + unit->unit_Buffer_Size, TAG_END );

				if ( ! pb )
				{
					unit->unit_JobError = ParErr_LineErr;
					break;
				}

				unit->unit_Buffer_Allocated++;
			}

			pb->pb_Pos = 0;
			pb->pb_Sent = 0;
			pb->pb_Free = unit->unit_Buffer_Size;
			unit->unit_Buffer_Active = pb;
		}

		len = MIN( size, pb->pb_Free );

		memcpy( &pb->pb_Data[pb->pb_Pos], data, len );

		data += len;
		size -= len;
		accepted += len;
		pb->pb_Pos += len;
		pb->pb_Free -= len;

		if ( ! pb->pb_Free )
		{
			Buffer_Flush( unit );
		}
	}

	return ( accepted );
}

// --
