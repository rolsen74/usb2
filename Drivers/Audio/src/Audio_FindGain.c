
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct AudioGainEntity
{
	U8 Source;
	U8 VolumeChannel;
	U8 StreamingOutput;
};

// --

SEC_CODE void
Audio_FindGain( struct AudioUnit *unit, U32 configSize )
{
	struct AudioGainEntity entities[256];

	const U8 *data;
	U32 length;
	U32 remaining;
	U32 index;
	U32 entity;
	U8 interface;
	U8 controlSize;
	S32 control;
	S32 associated;

	data = (PTR) unit->StartMsg->ConfigDescriptors;
	remaining = configSize;
	interface = 0;
	control = FALSE;
	associated = FALSE;

	for ( index = 0; index < 256; index++ )
	{
		entities[index].Source = 0;
		entities[index].VolumeChannel = 0;
		entities[index].StreamingOutput = FALSE;
	}

	while ( remaining >= 2 )
	{
		length = data[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			return;
		}

		if ( ( data[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			if ( associated )
			{
				break;
			}

			control = ( data[3] == 0 ) && ( data[5] == USBCLASS_Audio ) && ( data[6] == 1 ) && ( data[7] == 0 );
			interface = data[2];
		}
		else if ( ( control ) && ( length >= 4 ) && ( data[1] == 0x24 ) )
		{
			entity = data[3];

			if ( ( data[2] == 1 ) && ( length >= 8 ) && ( data[7] <= length - 8 ) )
			{
				for ( index = 0; index < data[7]; index++ )
				{
					if ( data[8 + index] == unit->InterfaceNr )
					{
						associated = TRUE;
						break;
					}
				}
			}
			else if ( associated )
			{
				if ( ( data[2] == 3 ) && ( length >= 9 ) )
				{
					entities[entity].Source = data[7];
					entities[entity].StreamingOutput = ( data[4] == 1 ) && ( data[5] == 1 );
				}
				else if ( ( data[2] == 5 ) && ( length >= 7 ) && ( data[4] == 1 ) )
				{
					entities[entity].Source = data[5];
				}
				else if ( ( data[2] == 6 ) && ( length >= 8 ) )
				{
					entities[entity].Source = data[4];
					controlSize = data[5];

					if ( ( controlSize ) && ( length >= 7U + controlSize ) && ( data[6] & 2 ) )
					{
						entities[entity].VolumeChannel = 1;
					}
					else if ( ( unit->Format.Channels == 1 ) && ( controlSize ) &&
						( length >= 7U + 2U * controlSize ) && ( data[6 + controlSize] & 2 ) )
					{
						entities[entity].VolumeChannel = 2;
					}
				}
			}
		}

		data += length;
		remaining -= length;
	}

	entity = unit->TerminalLink;

	if ( ( ! associated ) || ( ! entity ) || ( ! entities[entity].StreamingOutput ) )
	{
		return;
	}

	for ( index = 0; ( index < 255 ) && ( entity ); index++ )
	{
		if ( entities[entity].VolumeChannel )
		{
			unit->GainInterface = interface;
			unit->GainEntity = entity;
			unit->GainChannel = entities[entity].VolumeChannel - 1;

			return;
		}

		entity = entities[entity].Source;
	}
}

// --
