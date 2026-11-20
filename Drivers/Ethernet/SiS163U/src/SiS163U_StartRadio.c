
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_RF_CTRL		0x00fc
#define SIS163U_RF_CONFIG	0x005c

struct MACSetting
{
	U16 Register;
	U32 Value;
};

struct RFSetting
{
	U8 Register;
	U16 Value;
};

// --

static S32
WriteRFTable( struct USB2_EPResource *control, const struct RFSetting *settings, U32 count )
{
	U32 index;
	S32 error;

	for( index = 0; index < count; index++ )
	{
		error = SiS163U_WriteRF( control, settings[index].Register, settings[index].Value );

		if ( error )
		{
			USBERROR( "SiS163U: RF register $%02lx write failed (USB error %ld)",
				(U32) settings[index].Register, error );

			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static S32
Calibrate( struct USB2_EPResource *control, U32 mode )
{
	U32 index;

	if ( ( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, mode ) ) ||
		( ( mode == 0xa0 ) && ( SiS163U_WriteBaseband( control, 0xc2, 0x3f, TRUE ) ) ) ||
		( SiS163U_WriteRF( control, 0x07, 0x1000 ) ) )
	{
		return ( FALSE );
	}

	for( index = 0; index < 4; index++ )
	{
		if ( SiS163U_WriteRF( control, 0x07, 0x500d ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

S32
SiS163U_StartRadio( struct EthernetUnit *unit )
{
	static const U8 clearRegisters[] =
	{
		0x64, 0xd4, 0xd8, 0xf8,
	};
	static const struct MACSetting macDefaults[] =
	{
		{ 0x00c0, 0x08100130 }, { 0x00c4, 0x081004a0 },
		{ 0x00f0, 0x0000844f }, { 0x00f4, 0x00000040 },
		{ 0x0098, 0x04000000 }, { 0x0070, 0xea600000 },
		{ 0x0064, 0x00003300 }, { 0x0068, 0x00000040 },
		{ 0x0020, 0x08100810 }, { 0x0024, 0x07070000 },
		{ 0x009c, 0x00000064 }, { 0x00a4, 0x00000005 },
		{ 0x00a0, 0x54600001 },
	};
	static const struct RFSetting startup[] =
	{
		{ 0x1f, 0x0000 }, { 0x1f, 0x0000 }, { 0x1f, 0x0040 },
		{ 0x1f, 0x0060 }, { 0x1f, 0x0061 }, { 0x1f, 0x0061 },
		{ 0x00, 0x0400 }, { 0x1f, 0x0801 }, { 0x1f, 0x0841 }, { 0x1f, 0x0861 },
		{ 0x00, 0x2e36 }, { 0x01, 0x1300 }, { 0x02, 0x3d81 }, { 0x03, 0x18f5 },
		{ 0x04, 0x7fff }, { 0x05, 0x0000 }, { 0x06, 0x39b7 }, { 0x07, 0x0000 },
		{ 0x0a, 0x0000 }, { 0x0b, 0x0000 }, { 0x0c, 0x0000 }, { 0x0d, 0x0000 },
		{ 0x0e, 0x8e01 }, { 0x10, 0x01e0 }, { 0x11, 0x1000 }, { 0x13, 0x0c30 },
		{ 0x13, 0x4c30 }, { 0x14, 0x8f00 }, { 0x15, 0x01a0 }, { 0x16, 0x0000 },
		{ 0x17, 0x0c00 }, { 0x18, 0x0000 }, { 0x19, 0x0f00 }, { 0x08, 0x6013 },
		{ 0x07, 0x100d }, { 0x09, 0x2820 }, { 0x0b, 0x19d0 }, { 0x07, 0x000d },
		{ 0x07, 0x100d }, { 0x12, 0x341f }, { 0x12, 0x361f }, { 0x12, 0x349f },
		{ 0x12, 0x369f }, { 0x12, 0x2150 }, { 0x12, 0x2350 }, { 0x12, 0x21d0 },
		{ 0x12, 0x23d0 },
	};
	static const struct RFSetting finish[] =
	{
		{ 0x08, 0xe411 }, { 0x09, 0x0800 }, { 0x01, 0x13be }, { 0x19, 0x0f1f },
	};

	struct USB2_EPResource *control;

	const char *stage;
	U32 index;
	U32 revision;
	U32 selector;
	U32 value;

	control = unit->unit_USB_Register->Res_Control;
	revision = unit->unit_ChipRevision;
	stage = "MAC reset";

	if ( ( SiS163U_WriteRegister( control, 0x0014, 0 ) ) ||
		( SiS163U_WriteRegister( control, 0x0000, 0x50900100 ) ) ||
		( SiS163U_WriteRegister( control, 0x0070, 0x80000000 ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, 0 ) ) ||
		( SiS163U_WriteRegister( control, 0x0004, 0x00040000 ) ) ||
		( SiS163U_ReadRegister( control, 0x0000, & value ) ) ||
		( SiS163U_WriteRegister( control, 0x0000, value | 0x50000100 ) ) ||
		( SiS163U_StopRadio( unit ) ) )
	{
		goto failed;
	}

	for( index = 0; index < sizeof( clearRegisters ); index++ )
	{
		if ( SiS163U_WriteRegister( control, clearRegisters[index], 0 ) )
		{
			goto failed;
		}
	}

	if ( ( ( revision >= 0x30 ) && ( revision <= 0x33 ) ) || ( revision == 0x40 ) )
	{
		stage = "revision-specific RF interface setup";

		if ( ( SiS163U_ReadRegister( control, SIS163U_RF_CONFIG, & value ) ) ||
			( SiS163U_WriteRegister( control, SIS163U_RF_CONFIG, ( value & 0xfffcf8ff ) | 0x48000000 ) ) ||
			( SiS163U_WriteRegister( control, 0x00f8, 0x41 ) ) ||
			( SiS163U_ReadRegister( control, 0x0078, & value ) ) ||
			( SiS163U_WriteRegister( control, 0x0078, ( value & ~0x0eU ) | 1 ) ) )
		{
			goto failed;
		}

		// The type 8 RF interface uses two indexed words in the MAC.
		if ( ( SiS163U_ReadRegister( control, 0x0000, & selector ) ) ||
			( SiS163U_WriteRegister( control, 0x0000, ( selector & ~0x1eU ) | 0x5000001a ) ) ||
			( SiS163U_ReadRegister( control, 0x0004, & value ) ) ||
			( SiS163U_WriteRegister( control, 0x0004, ( value & 0xffff0000 ) | 0x3909 ) ) ||
			( SiS163U_WriteRegister( control, 0x0000, ( selector & ~0x1eU ) | 0x5000001e ) ) ||
			( SiS163U_ReadRegister( control, 0x0004, & value ) ) ||
			( SiS163U_WriteRegister( control, 0x0004, value & 0xffff0000 ) ) ||
			( SiS163U_WriteRegister( control, 0x0000, selector ) ) )
		{
			goto failed;
		}
	}

	stage = "MAC defaults";

	if ( ( SiS163U_ReadRegister( control, 0x00bc, & value ) ) ||
		( SiS163U_WriteRegister( control, 0x00bc, ( value & 0xfffff000 ) | 0x031f ) ) )
	{
		goto failed;
	}

	for( index = 0; index < sizeof( macDefaults ) / sizeof( macDefaults[0] ); index++ )
	{
		if ( SiS163U_WriteRegister( control, macDefaults[index].Register, macDefaults[index].Value ) )
		{
			goto failed;
		}
	}

	if ( ( SiS163U_ReadRegister( control, 0x006c, & value ) ) ||
		( SiS163U_WriteRegister( control, 0x006c, value | 0x20 ) ) )
	{
		goto failed;
	}

	// Type 8 uses the built-in table when the EEPROM has no custom table flag.
	if ( ( unit->unit_BoardOptions != 0xffff ) && ( ! ( unit->unit_BoardOptions & 1 ) ) )
	{
		if ( ( SiS163U_WriteRegister( control, 0x00d4, 0x87347002 ) ) ||
			( SiS163U_WriteRegister( control, 0x00d8, 0x01007200 ) ) )
		{
			goto failed;
		}

		USBINFO( "SiS163U: RF type $08 default table programmed" );
	}

	// Let the MAC and baseband settle before calibration.
	Delay( 1 );

	stage = "RF channel preset";

	if ( SiS163U_SetChannel( unit, 1 ) )
	{
		goto failed;
	}

	stage = "baseband setup";

	// Keep the station-mode bits set while toggling the baseband reset.
	value = SIS163U_MAC_RX_MODE;

	if ( ( SiS163U_WriteRegister( control, 0, value ) ) ||
		( SiS163U_WriteRegister( control, 0, value | 0x50000100 ) ) ||
		( SiS163U_WriteRegister( control, 0, value ) ) ||
		( SiS163U_WriteRegister( control, 0, value & ~0x00020000U ) ) ||
		( SiS163U_ReadRegister( control, 0, & value ) ) ||
		( SiS163U_InitBaseband( control ) ) )
	{
		goto failed;
	}

	USBINFO( "SiS163U: RF calibration MAC mode $%08lx", value );

	stage = "RF setup";

	if ( ! WriteRFTable( control, startup, sizeof( startup ) / sizeof( startup[0] ) ) )
	{
		goto failed;
	}

	stage = "RF calibration";

	if ( ( ! Calibrate( control, 0xa0 ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, 0 ) ) ||
		( SiS163U_WriteBaseband( control, 0xc2, 0, TRUE ) ) ||
		( ! WriteRFTable( control, finish, sizeof( finish ) / sizeof( finish[0] ) ) ) ||
		( ! Calibrate( control, 0x60 ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, 0 ) ) ||
		( SiS163U_ReadRegister( control, SIS163U_RF_CONFIG, & value ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CONFIG, ( value & 0xfffffd00 ) | 0xfd00 ) ) ||
		( SiS163U_ReadRegister( control, 0, & value ) ) ||
		( SiS163U_WriteRegister( control, 0, value | 0x50020000 ) ) )
	{
		goto failed;
	}

	stage = "RF calibration after enabling the transceiver";

	// Calibration writes the channel register directly.
	unit->unit_Channel = 0;

	if ( ( ! Calibrate( control, 0xa0 ) ) ||
		( SiS163U_WriteBaseband( control, 0xc2, 0, TRUE ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, 0 ) ) ||
		( SiS163U_SetChannel( unit, 1 ) ) )
	{
		goto failed;
	}

	unit->unit_Channel = 0;

	if ( ( ! Calibrate( control, 0x60 ) ) ||
		( SiS163U_WriteRegister( control, SIS163U_RF_CTRL, 0 ) ) )
	{
		goto failed;
	}

	stage = "initial channel selection";

	if ( ( SiS163U_SetChannel( unit, 1 ) ) ||
		( SiS163U_ReadRegister( control, 0x0064, & value ) ) ||
		( SiS163U_WriteRegister( control, 0x0064, value | 0x08 ) ) )
	{
		goto failed;
	}

	USBINFO( "SiS163U: RF preset and calibration completed on channel 1" );

	return ( TRUE );

failed:
	unit->unit_Channel = 0;

	USBERROR( "SiS163U: RF type $08 initialization failed during %s", stage );

	SiS163U_StopRadio( unit );

	return ( FALSE );
}

// --
