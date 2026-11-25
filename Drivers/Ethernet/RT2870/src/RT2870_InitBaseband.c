
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_HOST_COMMAND		0x0404
#define RT2870_MAC_STATUS		0x1200
#define RT2870_MAC_STATUS_BUSY		0x00000003
#define RT2870_BBP_CSR			0x101c
#define RT2870_BBP_REG_SHIFT		8
#define RT2870_BBP_READ			0x00010000
#define RT2870_BBP_BUSY			0x00020000
#define RT2870_BBP_PARALLEL		0x00080000
#define RT2870_MCU_MAILBOX		0x7010
#define RT2870_MCU_INTERRUPT		0x7024
#define RT2870_MCU_BBP_AGENT		0x7028
#define RT2870_MCU_MAILBOX_OWNER	0x01000000
#define RT2870_MCU_BOOT_SIGNAL		0x72
#define RT2870_EEPROM_NIC_CONFIG1	0x0036
#define RT2870_EEPROM_LNA		0x0044
#define RT2870_EEPROM_EXTERNAL_LNA	0x0004
#define RT2870_EEPROM_NIC_CONFIG	0x0034
#define RT2870_EEPROM_BBP_BASE		0x00f0
#define RT2870_EEPROM_BBP_WORDS		16
#define RT2860D_BBP_VERIFY_REGISTER	84

// --

// RT2860D defaults follow Linux rt2800_init_bbp_28xx.
static const struct
{
	U8 reg;
	U8 value;
} RT2860DBaseband[] =
{
	{  65, 0x2c },
	{  66, 0x38 },
	{  69, 0x12 },
	{  73, 0x10 },
	{  70, 0x0a },
	{  81, 0x37 },
	{  82, 0x62 },
	{  83, 0x6a },
	{  84, 0x19 },
	{  86, 0x00 },
	{  91, 0x04 },
	{  92, 0x00 },
	{ 103, 0x00 },
	{ 105, 0x05 },
	{ 106, 0x35 }
};

// --

static S32
BasebandAccess( struct USB2_EPResource *control, U8 reg, U32 flags, U32 *csr )
{
	S32 error;

	error = RT2870_WaitRegisterIdle( control, RT2870_BBP_CSR, RT2870_BBP_BUSY, csr );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_BBP_CSR,
			RT2870_BBP_PARALLEL | RT2870_BBP_BUSY | ( (U32) reg << RT2870_BBP_REG_SHIFT ) | flags );
	}

	if ( ! error )
	{
		error = RT2870_WaitRegisterIdle( control, RT2870_BBP_CSR, RT2870_BBP_BUSY, csr );
	}

	return ( error );
}

// --

static S32
ConfigureReceiveGain( struct USB2_EPResource *control )
{
	U16 word;
	U8 lna;
	U8 reg;
	U32 csr;
	S32 error;

	error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
		RT2870_EEPROM_NIC_CONFIG1, & word, sizeof( word ) );

	if ( error )
	{
		return ( error );
	}

	word = LE_SWAP16( word );

	error = BasebandAccess( control, 75, ( word & RT2870_EEPROM_EXTERNAL_LNA ) ? 0x46 : 0x50, & csr );

	if ( ! error )
	{
		error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
			RT2870_EEPROM_LNA, & word, sizeof( word ) );
	}

	if ( error )
	{
		return ( error );
	}

	lna = LE_SWAP16( word ) & 0xff;

	if ( lna > 0x37 )
	{
		lna = 0;
	}

	for( reg = 62; reg <= 64; reg++ )
	{
		error = BasebandAccess( control, reg, 0x37 - lna, & csr );

		if ( error )
		{
			return ( error );
		}
	}

	error = BasebandAccess( control, 66, 0x2e + lna, & csr );

	if ( ! error )
	{
		error = BasebandAccess( control, 3, RT2870_BBP_READ, & csr );
	}

	if ( ! error )
	{
		// BBP3 bits 4:3 select the number of receive chains.
		error = BasebandAccess( control, 3, ( csr & 0xe7 ) | 0x08, & csr );
	}

	return ( error );
}

// --

static S32
ConfigureBaseband( struct USB2_EPResource *control )
{
	U32 index;
	U32 csr;
	U32 overrides;
	U16 antenna;
	U16 word;
	U8 reg;
	U8 value;
	U8 expected;
	S32 error;

	overrides = 0;
	expected = 0;

	error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
		RT2870_EEPROM_NIC_CONFIG, & antenna, sizeof( antenna ) );

	if ( error )
	{
		return ( FALSE );
	}

	antenna = LE_SWAP16( antenna );

	USBINFO( "RT2870: EEPROM NIC config $%04lx, RF type $%lx, %lu TX and %lu RX chains",
		(U32) antenna, (U32) ( ( antenna >> 8 ) & 0xf ),
		(U32) ( ( antenna >> 4 ) & 0xf ), (U32) ( antenna & 0xf ) );

	if ( ( antenna & 0x0fff ) != 0x0122 )
	{
		USBERROR( "RT2870: unsupported RF configuration, expected 2T2R RF2820" );

		return ( FALSE );
	}

	for( index = 0; index < sizeof( RT2860DBaseband ) / sizeof( RT2860DBaseband[0] ); index++ )
	{
		reg = RT2860DBaseband[index].reg;
		value = RT2860DBaseband[index].value;

		error = BasebandAccess( control, reg, value, & csr );

		if ( error )
		{
			USBERROR( "RT2870: BBP register %lu default write failed, error %ld", (U32) reg, error );

			return ( FALSE );
		}

		if ( reg == RT2860D_BBP_VERIFY_REGISTER )
		{
			expected = value;
		}
	}

	for( index = 0; index < RT2870_EEPROM_BBP_WORDS; index++ )
	{
		error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
			RT2870_EEPROM_BBP_BASE + index * sizeof( word ), & word, sizeof( word ) );

		if ( error )
		{
			return ( FALSE );
		}

		word = LE_SWAP16( word );
		reg = word >> 8;
		value = word & 0xff;

		if ( ( ! reg ) || ( reg == 0xff ) )
		{
			continue;
		}

		error = BasebandAccess( control, reg, value, & csr );

		if ( error )
		{
			USBERROR( "RT2870: BBP register %lu EEPROM override failed, error %ld", (U32) reg, error );

			return ( FALSE );
		}

		if ( reg == RT2860D_BBP_VERIFY_REGISTER )
		{
			expected = value;
		}

		overrides++;
	}

	error = BasebandAccess( control, RT2860D_BBP_VERIFY_REGISTER, RT2870_BBP_READ, & csr );

	if ( error )
	{
		USBERROR( "RT2870: BBP register 84 readback failed, error %ld", error );

		return ( FALSE );
	}

	value = csr & 0xff;

	if ( value != expected )
	{
		USBERROR( "RT2870: BBP register 84 read back $%02lx, expected $%02lx", (U32) value, (U32) expected );

		return ( FALSE );
	}

	USBINFO( "RT2870: BBP configured, %lu EEPROM overrides, register 84 verified as $%02lx",
		overrides, (U32) value );

	error = ConfigureReceiveGain( control );

	if ( error )
	{
		USBERROR( "RT2870: receive gain setup failed, error %ld", error );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
RT2870_InitBaseband( struct USB2_EPResource *control )
{
	U32 attempt;
	U32 csr;
	U32 mailbox;
	S32 error;
	U8 revision;

	error = RT2870_ResetMAC( control );

	if ( ! error )
	{
		error = RT2870_WaitRegisterIdle( control, RT2870_MAC_STATUS, RT2870_MAC_STATUS_BUSY, & csr );
	}

	if ( error )
	{
		USBERROR( "RT2870: MAC/baseband reset failed, error %ld", error );

		return ( FALSE );
	}

	error = RT2870_WriteRegister( control, RT2870_MCU_BBP_AGENT, 0 );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_MCU_INTERRUPT, 0 );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_MCU_MAILBOX, RT2870_MCU_MAILBOX_OWNER );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_HOST_COMMAND, RT2870_MCU_BOOT_SIGNAL );
	}

	if ( error )
	{
		USBERROR( "RT2870: baseband boot signal failed, USB error %ld", error );

		return ( FALSE );
	}

	for( attempt = 0; attempt < 20; attempt++ )
	{
		Delay( 1 );

		error = BasebandAccess( control, 0, RT2870_BBP_READ, & csr );

		if ( error )
		{
			USBERROR( "RT2870: baseband revision read failed, error %ld", error );

			return ( FALSE );
		}

		revision = csr & 0xff;

		if ( ( revision != 0 ) && ( revision != 0xff ) )
		{
			USBINFO( "RT2870: BBP register 0 revision $%02lx", (U32) revision );

			return ( ConfigureBaseband( control ) );
		}
	}

	USBERROR( "RT2870: baseband did not report a valid revision, BBP_CSR $%08lx", csr );

	if ( ! RT2870_ReadRegister( control, RT2870_MCU_MAILBOX, & mailbox ) )
	{
		USBINFO( "RT2870: MCU mailbox after baseband check $%08lx", mailbox );
	}

	return ( FALSE );
}

// --
