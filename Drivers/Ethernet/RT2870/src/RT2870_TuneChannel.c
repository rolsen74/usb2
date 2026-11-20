
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_RF_CSR			0x1020
#define RT2870_RF_BUSY			0x80000000
#define RT2870_RF2_DISABLE_RX3		0x00000040
#define RT2870_RF3_CALIBRATE		0x00000004
#define RT2870_RF3_TX_POWER		0x00003e00
#define RT2870_RF4_TX_POWER		0x000007c0
#define RT2870_RF4_FREQ_OFFSET		0x001f8000
#define RT2870_RF4_HT40			0x00200000
#define RT2870_EEPROM_FREQ		0x003a
#define RT2870_EEPROM_TX_POWER1		0x0052
#define RT2870_EEPROM_TX_POWER2		0x0060

// --

// RF2820 channel words for 20 MHz operation, from Linux rt2800.
static const struct
{
	U32 rf2;
	U32 rf4;
} RF2820Channels[] =
{
	{ 0x184c0786, 0x1800510b },
	{ 0x184c0786, 0x1800519f },
	{ 0x184c078a, 0x1800518b },
	{ 0x184c078a, 0x1800519f },
	{ 0x184c078e, 0x1800518b },
	{ 0x184c078e, 0x1800519f },
	{ 0x184c0792, 0x1800518b },
	{ 0x184c0792, 0x1800519f },
	{ 0x184c0796, 0x1800518b },
	{ 0x184c0796, 0x1800519f },
	{ 0x184c079a, 0x1800518b },
	{ 0x184c079a, 0x1800519f },
	{ 0x184c079e, 0x1800518b }
};

// --

S32
RT2870_TuneChannel( struct USB2_EPResource *control, U32 channel )
{
	U32 rf[4];
	U32 csr;
	U32 phase;
	U32 index;
	U32 channelIndex;
	U32 powerByte;
	U16 freq;
	U16 power1;
	U16 power2;
	S32 error;

	if ( ( channel < 1 ) || ( channel > sizeof( RF2820Channels ) / sizeof( RF2820Channels[0] ) ) )
	{
		return ( FALSE );
	}

	channelIndex = channel - 1;
	powerByte = ( channelIndex & 1 ) * 8;

	error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
		RT2870_EEPROM_FREQ, & freq, sizeof( freq ) );

	if ( ! error )
	{
		error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
			RT2870_EEPROM_TX_POWER1 + ( channelIndex & ~1U ), & power1, sizeof( power1 ) );
	}

	if ( ! error )
	{
		error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
			RT2870_EEPROM_TX_POWER2 + ( channelIndex & ~1U ), & power2, sizeof( power2 ) );
	}

	if ( error )
	{
		USBERROR( "RT2870: could not read channel %lu RF calibration, USB error %ld", channel, error );

		return ( FALSE );
	}

	freq = LE_SWAP16( freq ) & 0xff;
	power1 = ( LE_SWAP16( power1 ) >> powerByte ) & 0xff;
	power2 = ( LE_SWAP16( power2 ) >> powerByte ) & 0xff;

	if ( freq == 0xff )
	{
		freq = 0;
	}

	freq &= RT2870_RF4_FREQ_OFFSET >> 15;

	if ( power1 > 31 )
	{
		power1 = 0;
	}

	if ( power2 > 31 )
	{
		power2 = 0;
	}

	rf[0] = 0x18402ecc;
	rf[1] = RF2820Channels[channelIndex].rf2 | RT2870_RF2_DISABLE_RX3;
	rf[2] = ( 0x18168a55 & ~( RT2870_RF3_TX_POWER | RT2870_RF3_CALIBRATE ) ) |
		( ( (U32) power1 << 9 ) & RT2870_RF3_TX_POWER );
	rf[3] = ( RF2820Channels[channelIndex].rf4 & ~( RT2870_RF4_TX_POWER | RT2870_RF4_FREQ_OFFSET | RT2870_RF4_HT40 ) ) |
		( ( (U32) power2 << 6 ) & RT2870_RF4_TX_POWER ) |
		( ( (U32) freq << 15 ) & RT2870_RF4_FREQ_OFFSET );

	error = RT2870_WaitRegisterIdle( control, RT2870_RF_CSR, RT2870_RF_BUSY, & csr );

	if ( error )
	{
		USBERROR( "RT2870: RF2820 controller unavailable, error %ld", error );

		return ( FALSE );
	}

	// Latch the channel by pulsing RF3 bit 2 across three passes.
	for( phase = 0; phase < 3; phase++ )
	{
		rf[2] &= ~RT2870_RF3_CALIBRATE;
		rf[2] |= ( phase == 1 ) ? RT2870_RF3_CALIBRATE : 0;

		for( index = 0; index < sizeof( rf ) / sizeof( rf[0] ); index++ )
		{
			error = RT2870_WriteRegister( control, RT2870_RF_CSR, RT2870_RF_BUSY | rf[index] );

			if ( ! error )
			{
				error = RT2870_WaitRegisterIdle( control, RT2870_RF_CSR, RT2870_RF_BUSY, & csr );
			}

			if ( error )
			{
				USBERROR( "RT2870: RF2820 channel %lu pass %lu register %lu failed, error %ld",
					channel, phase + 1, index + 1, error );

				return ( FALSE );
			}
		}

		Delay( 1 );
	}

	USBINFO( "RT2870: RF2820 channel %lu programmed, RF_CSR $%08lx, frequency offset $%02lx",
		channel, csr, (U32) freq );

	return ( TRUE );
}

// --
