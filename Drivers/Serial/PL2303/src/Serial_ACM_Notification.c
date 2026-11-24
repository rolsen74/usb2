
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define ACM_NOTIFICATION_HEADER 8U
#define ACM_SERIAL_STATE 0x20

SEC_CODE void
Serial_ACM_Notification( struct SerialContext *ctx, struct SerialUnit *unit, const U8 *data, U32 length )
{
	U8 *packet;
	U32 used;
	U32 total;
	U32 index;

	packet = unit->ACMNotification;
	used = unit->ACMNotificationUsed;
	total = unit->ACMNotificationSize;

	// An eight-byte interrupt endpoint can split the header from the UART state.
	for( index = 0; index < length; index++ )
	{
		if ( used < sizeof( unit->ACMNotification ) )
		{
			packet[used] = data[index];
		}

		used++;

		if ( used == ACM_NOTIFICATION_HEADER )
		{
			total = ACM_NOTIFICATION_HEADER + ( packet[6] | ( (U32) packet[7] << 8 ) );
		}

		if ( used == total )
		{
			if ( ( total == sizeof( unit->ACMNotification ) ) && ( packet[0] == 0xa1 ) &&
				( packet[1] == ACM_SERIAL_STATE ) && ( ! packet[2] ) && ( ! packet[3] ) &&
				( packet[4] == unit->StartMsg->InterfaceDescriptor->InterfaceNumber ) && ( ! packet[5] ) )
			{
				unit->UartState = packet[8] & 0x7f;

				if ( unit->UartState & SERIAL_UART_BREAK )
				{
					Serial_Read_Break( ctx, unit );
				}
			}

			used = 0;
			total = 0;
		}
	}

	unit->ACMNotificationUsed = used;
	unit->ACMNotificationSize = total;
}

// --
