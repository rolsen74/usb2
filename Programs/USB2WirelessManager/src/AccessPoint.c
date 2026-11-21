
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
PrintSSID( const U8 *ssid, U32 length )
{
	U32 index;
	U8 byte;

	fputc( '"', stderr );

	for( index = 0; index < length; index++ )
	{
		byte = ssid[index];

		if ( ( byte <= 0x20 ) || ( byte >= 0x7f ) || ( byte == '"' ) || ( byte == '\\' ) )
		{
			fprintf( stderr, "\\x%02X", (unsigned) byte );
		}
		else
		{
			fputc( byte, stderr );
		}
	}

	fprintf( stderr, "\" (%lu bytes)", (unsigned long) length );
}

// --

static void
ReportBeacon( struct Wireless *wireless, const U8 *bssid, const U8 *ssid, U32 length, U32 advertised )
{
	U32 index;

	if ( wireless->ScanReportedCount >= WIRELESS_SCAN_REPORT_LIMIT )
	{
		return;
	}

	for( index = 0; index < wireless->ScanReportedCount; index++ )
	{
		if ( ! memcmp( wireless->ScanReportedBSSIDs[index], bssid, WIRELESS_ADDR_SIZE ) )
		{
			return;
		}
	}

	memcpy( wireless->ScanReportedBSSIDs[index], bssid, WIRELESS_ADDR_SIZE );

	wireless->ScanReportedCount++;

	fprintf( stderr, "Heard %02X:%02X:%02X:%02X:%02X:%02X on channel %u",
		(unsigned) bssid[0], (unsigned) bssid[1], (unsigned) bssid[2],
		(unsigned) bssid[3], (unsigned) bssid[4], (unsigned) bssid[5],
		(unsigned) wireless->AP.Channel );

	if ( advertised && ( advertised != wireless->AP.Channel ) )
	{
		fprintf( stderr, ", advertises channel %u", (unsigned) advertised );
	}

	fprintf( stderr, ", SSID " );

	PrintSSID( ssid, length );

	fputc( '\n', stderr );
}

// --

#ifndef USB2WIFI_EMBEDDED

void
Wireless_Report_Networks( const struct WiFiScan *scan )
{
	const struct WiFiNetwork *network;

	U32 index;

	for( index = 0; index < scan->Count; index++ )
	{
		network = & scan->Network[index];

		fprintf( stderr, "Channel %u, SSID ", (unsigned) network->Channel );

		PrintSSID( (const U8 *) network->SSID, strlen( network->SSID ) );

		fputc( '\n', stderr );
	}
}

#endif

// --

void
Wireless_Report_Scan( const struct Wireless *wireless )
{
	fprintf( stderr, "Received %lu beacon frames while looking for SSID ",
		(unsigned long) wireless->ScanBeaconCount );

	PrintSSID( (const U8 *) wireless->SSID, wireless->SSIDLength );

	fputc( '\n', stderr );

	if ( wireless->ScanMalformedMatches || wireless->ScanOtherChannelMatches )
	{
		fprintf( stderr, "Exact SSID matches rejected, %lu malformed, %lu on a different advertised channel.\n",
			(unsigned long) wireless->ScanMalformedMatches, (unsigned long) wireless->ScanOtherChannelMatches );
	}
}

// --

static S32
SelectSecurity( struct Wireless *wireless, const struct AccessPoint *candidate )
{
	if ( wireless->UseWPA )
	{
		if ( WPA_Supports_TKIP( candidate->WPAIE, candidate->WPAIELength ) )
		{
			return ( TRUE );
		}

		wireless->ScanSecurityMismatch++;

		return ( FALSE );
	}

	if ( ( ! wireless->AllowWPA2 ) || ( ! wireless->AllowWPA3 ) )
	{
		return ( TRUE );
	}

	if ( ( candidate->Protected ) && ( candidate->RSNLength >= 2 ) )
	{
		if ( RSN_Supports_SAE( candidate->RSN + 2, candidate->RSNLength - 2 ) )
		{
			wireless->UseSAE = TRUE;

			return ( TRUE );
		}

		if ( RSN_Supports_WPA2( candidate->RSN + 2, candidate->RSNLength - 2 ) )
		{
			wireless->UseSAE = FALSE;

			return ( TRUE );
		}
	}

	wireless->ScanSecurityMismatch++;

	return ( FALSE );
}

// --

static U8
NetworkSecurity( const struct Wireless *wireless, const struct AccessPoint *candidate )
{
	if ( ( ! wireless->ScanRT2870 ) && ( ! candidate->RatesSupported ) )
	{
		return ( WIFI_SECURITY_UNSUPPORTED );
	}

	if ( candidate->RSNLength >= 2 )
	{
		if ( RSN_Supports_SAE( candidate->RSN + 2, candidate->RSNLength - 2 ) )
		{
			return ( USB2WIFI_WPA3 );
		}

		if ( RSN_Supports_WPA2( candidate->RSN + 2, candidate->RSNLength - 2 ) )
		{
			return ( USB2WIFI_WPA2 );
		}
	}

	if ( WPA_Supports_TKIP( candidate->WPAIE, candidate->WPAIELength ) )
	{
		return ( USB2WIFI_WPA );
	}

	return ( ( candidate->Protected || wireless->ScanRT2870 ) ? WIFI_SECURITY_UNSUPPORTED : USB2WIFI_OPEN );
}

// --

static void
RecordNetwork( struct Wireless *wireless, const struct AccessPoint *candidate, const U8 *ssid, U32 length )
{
	struct WiFiScan *scan;
	struct WiFiNetwork *network;

	U32 index;
	U8 security;

	if ( ( ! ssid ) || ( ! length ) || ( length > WIRELESS_MAX_SSID ) || memchr( ssid, 0, length ) )
	{
		return;
	}

	scan = wireless->ScanNetworks;
	security = NetworkSecurity( wireless, candidate );

	for( index = 0; index < scan->Count; index++ )
	{
		network = & scan->Network[index];

		if ( ( strlen( network->SSID ) == length ) && ( ! memcmp( network->SSID, ssid, length ) ) )
		{
			if ( ( security != WIFI_SECURITY_UNSUPPORTED ) &&
				( network->Security == WIFI_SECURITY_UNSUPPORTED || security > network->Security ) )
			{
				network->Security = security;
				network->Channel = candidate->Channel;
				scan->Changed = index + 1;
			}

			return;
		}
	}

	if ( scan->Count < WIFI_SCAN_NETWORKS )
	{
		network = & scan->Network[scan->Count++];

		memcpy( network->SSID, ssid, length );

		network->SSID[length] = 0;
		network->Security = security;
		network->Channel = candidate->Channel;
		scan->Changed = scan->Count;
	}
}

// --

S32
Wireless_Match_Beacon( struct Wireless *wireless, const U8 *frame, U32 length )
{
	static const U8 legacy[] = { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 };
	static const U8 wpa[] = { 0, 0x50, 0xf2, 1 };
	struct AccessPoint candidate;

	const U8 *ssid;
	U32 ssidLength;
	U32 advertised;
	U32 offset;
	U32 end;
	U32 index;
	U32 rate;
	U32 rateLimit;
	U8 rates[sizeof( legacy )];
	S32 ssid_matches;
	S32 malformed;
	S32 otherChannel;
	U8 security;

	if ( wireless->ScanRT2870 )
	{
		wireless->ScanBeaconCount++;
	}

	if ( ( length >= 36 ) && ( frame[0] == 0x50 ) &&
		( memcmp( frame + 4, wireless->Station, WIRELESS_ADDR_SIZE ) ) )
	{
		return ( FALSE );
	}

	if ( ( length < 36 ) || ( ( frame[0] != 0x80 ) && ( frame[0] != 0x50 ) ) ||
		( frame[1] & 0x43 ) || ( ( frame[34] & 3 ) != 1 ) || ( frame[16] & 1 ) ||
		( memcmp( frame + 10, frame + 16, WIRELESS_ADDR_SIZE ) ) )
	{
		return ( FALSE );
	}

	memset( & candidate, 0, sizeof( candidate ) );

	memset( rates, 0, sizeof( rates ) );

	rateLimit = wireless->ScanRT2870 ? sizeof( legacy ) : 4;
	candidate.Channel = wireless->AP.Channel;
	candidate.Protected = ( frame[34] & 0x10 ) != 0;
	candidate.RatesSupported = TRUE;
	ssid_matches = FALSE;
	ssid = NULL;
	ssidLength = 0;
	advertised = 0;
	malformed = FALSE;
	otherChannel = FALSE;

	for( offset = 36; length - offset >= 2; offset = end )
	{
		end = offset + 2 + frame[offset + 1];

		if ( end > length )
		{
			malformed = TRUE;
			break;
		}

		switch( frame[offset] )
		{
			case 0:
			{
				ssidLength = frame[offset + 1];
				ssid = frame + offset + 2;
				ssid_matches = ( ssidLength == wireless->SSIDLength ) &&
					( ! memcmp( ssid, wireless->SSID, wireless->SSIDLength ) );

				break;
			}

			case 1:
			case 50:
			{
				for( index = offset + 2; index < end; index++ )
				{
					for( rate = 0; rate < rateLimit; rate++ )
					{
						if ( ( frame[index] & 0x7f ) == legacy[rate] )
						{
							rates[rate] |= frame[index];
							break;
						}
					}

					if ( ( rate == rateLimit ) && ( frame[index] & 0x80 ) )
					{
						candidate.RatesSupported = FALSE;
					}
				}

				break;
			}

			case 3:
			{
				if ( frame[offset + 1] != 1 )
				{
					malformed = TRUE;
				}
				else
				{
					advertised = frame[offset + 2];
					otherChannel |= advertised != candidate.Channel;
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

			case 244:
			{
				if ( ( frame[offset + 1] ) && ( end - offset <= sizeof( candidate.RSNX ) ) )
				{
					candidate.RSNXLength = end - offset;

					memcpy( candidate.RSNX, frame + offset, candidate.RSNXLength );
				}

				break;
			}

			case 221:
			{
				if ( ( frame[offset + 1] >= sizeof( wpa ) ) && ( ! memcmp( frame + offset + 2, wpa, sizeof( wpa ) ) ) )
				{
					candidate.Protected = TRUE;
					candidate.WPAIELength = end - offset;

					memcpy( candidate.WPAIE, frame + offset, candidate.WPAIELength );
				}

				break;
			}
		}
	}

	if ( malformed || ( offset != length ) )
	{
		if ( wireless->ScanRT2870 && ssid_matches )
		{
			wireless->ScanMalformedMatches++;
		}

		return ( FALSE );
	}

	if ( ( wireless->ScanNetworks ) && ( otherChannel ) )
	{
		return ( FALSE );
	}

	if ( ( ! wireless->ScanNetworks ) && ( wireless->ScanRT2870 ) && ( ssid ) && ( ssidLength <= WIRELESS_MAX_SSID ) &&
		( ( ! ssid_matches ) || ( otherChannel ) ) )
	{
		ReportBeacon( wireless, frame + 16, ssid, ssidLength, advertised );
	}

	if ( ( ! wireless->ScanNetworks ) && ( ! ssid_matches || otherChannel ) )
	{
		if ( wireless->ScanRT2870 && ssid_matches )
		{
			wireless->ScanOtherChannelMatches++;
		}

		return ( FALSE );
	}

	for( rate = 0; rate < rateLimit; rate++ )
	{
		if ( rates[rate] )
		{
			candidate.Rates[candidate.RateCount++] = rates[rate];
		}
	}

	candidate.RatesSupported &= rates[0] != 0;

	memcpy( candidate.BSSID, frame + 16, sizeof( candidate.BSSID ) );

	if ( wireless->ScanNetworks )
	{
		RecordNetwork( wireless, & candidate, ssid, ssidLength );

		return ( FALSE );
	}

	if ( wireless->DetectSecurity )
	{
		security = NetworkSecurity( wireless, & candidate );

		if ( ( security == WIFI_SECURITY_UNSUPPORTED ) ||
			( security == USB2WIFI_OPEN && wireless->Auto->Profile.Password[0] ) )
		{
			wireless->RejectedAP = candidate;
			wireless->ScanSecurityMismatch++;

			return ( FALSE );
		}

		wireless->Auto->Profile.Security = security;
		wireless->UseWPA = security == USB2WIFI_WPA;
		wireless->AllowWPA2 = security == USB2WIFI_WPA2;
		wireless->UseSAE = security == USB2WIFI_WPA3;
		wireless->AllowWPA3 = wireless->UseSAE;
	}

	if ( ! SelectSecurity( wireless, & candidate ) )
	{
		wireless->RejectedAP = candidate;

		return ( FALSE );
	}

	if ( wireless->DetectSecurity )
	{
		wireless->Auto->DetectSecurity = FALSE;
	}

	wireless->AP = candidate;

	return ( TRUE );
}
