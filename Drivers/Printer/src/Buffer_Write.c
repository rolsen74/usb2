
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

S32 Buffer_Write( struct PrinterUnit *unit, U8 *data, U32 size )
{
struct PrtBuffer *pb;
S32 retval;
U32 len;

	retval = FALSE;

	while( TRUE )
	{
		USBERROR( "Buffer_Write : Data %p : Size %lu", data, size );

		if ( size <= 0 )
		{
			break;
		}

		pb = unit->unit_Buffer_Active;

		while( ! pb )
		{
			pb = (PTR) RemHead( unit->unit_Buffer_FreeList );

			if ( pb )
			{
				unit->unit_Buffer_FreeCount--;
				break;
			}

			unit->unit_Buffer_FreeCount = 0;

			pb = AllocVecTags( sizeof( struct PrtBuffer ) + unit->unit_Buffer_Size, TAG_END );

			if ( ! pb )
			{
				USBERROR( "Printer : Error allocing memory" );
				goto bailout;
			}

			pb->pb_Pos	= 0;
			pb->pb_Free	= unit->unit_Buffer_Size;
			unit->unit_Buffer_Active = pb;
			break;
		}

		len	= MIN( size, pb->pb_Free );
		memcpy( & pb->pb_Data[pb->pb_Pos], data, len );

	#if 0
	{
	U32 cnt;
	U32 line;
	U8 *buf = data;

		cnt = 0;
		line = 1;

		while( TRUE )
		{
			if ( cnt >= 1024 )
			{
				break;
			}

			if ( cnt >= len )
			{
				break;
			}

			if ( line > 32 )
			{
				DebugPrintF( "\n" );
				line = 0;
			}

			DebugPrintF( "%02x ", buf[cnt] );

			line++;
			cnt++;
		}

		DebugPrintF( "\n" );
	}
	#endif

		data		+= len;
		size		-= len;
		pb->pb_Pos	+= len;
		pb->pb_Free	-= len;

		if ( pb->pb_Free <= 0 )
		{
			Buffer_Flush( unit );
		}
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --
