
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
Wireless_Match_Beacon( struct Wireless *wireless, const U8 *frame, U32 length )
{
	static const U8 cck[] = { 2, 4, 11, 22 };
	static const U8 wpa[] = { 0, 0x50, 0xf2, 1 };
	struct AccessPoint candidate;

	U32 offset;
	U32 end;
	U32 index;
	U32 rate;
	U8 rates[sizeof( cck )];
	S32 ssid_matches;

	if ( ( length < 36 ) || ( ( frame[0] != 0x80 ) && ( frame[0] != 0x50 ) ) ||
		( frame[1] & 0x43 ) || ( ( frame[34] & 3 ) != 1 ) || ( frame[16] & 1 ) ||
		( memcmp( frame + 10, frame + 16, WIRELESS_ADDR_SIZE ) ) )
	{
		return ( FALSE );
	}

	memset( & candidate, 0, sizeof( candidate ) );

	memset( rates, 0, sizeof( rates ) );

	candidate.Channel = wireless->AP.Channel;
	candidate.Protected = ( frame[34] & 0x10 ) != 0;
	candidate.RatesSupported = TRUE;
	ssid_matches = FALSE;

	for( offset = 36; length - offset >= 2; offset = end )
	{
		end = offset + 2 + frame[offset + 1];

		if ( end > length )
		{
			return ( FALSE );
		}

		switch( frame[offset] )
		{
			case 0:
			{
				ssid_matches = ( frame[offset + 1] == wireless->SSIDLength ) &&
					( ! memcmp( frame + offset + 2, wireless->SSID, wireless->SSIDLength ) );

				break;
			}

			case 1:
			case 50:
			{
				for( index = offset + 2; index < end; index++ )
				{
					for( rate = 0; rate < sizeof( cck ); rate++ )
					{
						if ( ( frame[index] & 0x7f ) == cck[rate] )
						{
							rates[rate] |= frame[index];
							break;
						}
					}

					if ( ( rate == sizeof( cck ) ) && ( frame[index] & 0x80 ) )
					{
						candidate.RatesSupported = FALSE;
					}
				}

				break;
			}

			case 3:
			{
				if ( ( frame[offset + 1] != 1 ) || ( frame[offset + 2] != candidate.Channel ) )
				{
					return ( FALSE );
				}

				break;
			}

			case 48:
			{
				candidate.Protected = TRUE;

				candidate.RSNLength = end - offset;

				memcpy( candidate.RSN, frame + offset, candidate.RSNLength );

				break;
			}

			case 221:
			{
				if ( ( frame[offset + 1] >= sizeof( wpa ) ) && ( ! memcmp( frame + offset + 2, wpa, sizeof( wpa ) ) ) )
				{
					candidate.Protected = TRUE;
				}

				break;
			}
		}
	}

	if ( ( ! ssid_matches ) || ( offset != length ) )
	{
		return ( FALSE );
	}

	for( rate = 0; rate < sizeof( cck ); rate++ )
	{
		if ( rates[rate] )
		{
			candidate.Rates[candidate.RateCount++] = rates[rate];
		}
	}

	candidate.RatesSupported &= rates[0] != 0;

	memcpy( candidate.BSSID, frame + 16, sizeof( candidate.BSSID ) );

	wireless->AP = candidate;

	return ( TRUE );
}
