
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
** OV534/OV772x register sequences adapted from Linux gspca/ov534.c.
** Copyright (C) 2008 Antonio Ospite <ospite@studenti.unina.it>
** Copyright (C) 2008 Jim Paris <jim@jtan.com>
** Copyright (C) 2009 Jean-Francois Moine http://moinejf.free.fr
** Based on a prototype by Mark Ferrell. PS3 Eye enhancements by Richard Kaswy,
** Max Thrun and Joseph Howse. Upstream license is GPL-2.0-or-later.
*/

// --

#include "inc/All.h"

// --

#define OV534_SENSOR_ADDRESS 0xf1
#define OV534_SENSOR_REGISTER 0xf2
#define OV534_SENSOR_WRITE 0xf3
#define OV534_SENSOR_READ 0xf4
#define OV534_SENSOR_OPERATION 0xf5
#define OV534_SENSOR_STATUS 0xf6
#define OV534_SENSOR_WRITE_REG 0x37
#define OV534_SENSOR_SELECT_REG 0x33
#define OV534_SENSOR_READ_REG 0xf9

static const U8 BridgeInit[][2] =
{
	{ 0x88, 0xf8 }, { 0x89, 0xff }, { 0x76, 0x03 }, { 0x92, 0x01 },
	{ 0x93, 0x18 }, { 0x94, 0x10 }, { 0x95, 0x10 }, { 0xe2, 0x00 },
	{ 0xe7, 0x3e }, { 0x96, 0x00 }, { 0x97, 0x20 }, { 0x97, 0x20 },
	{ 0x97, 0x20 }, { 0x97, 0x0a }, { 0x97, 0x3f }, { 0x97, 0x4a },
	{ 0x97, 0x20 }, { 0x97, 0x15 }, { 0x97, 0x0b }, { 0x8e, 0x40 },
	{ 0x1f, 0x81 }, { 0x34, 0x05 }, { 0xe3, 0x04 }, { 0x89, 0x00 },
	{ 0x76, 0x00 }, { 0xe7, 0x2e }, { 0x31, 0xf9 }, { 0x25, 0x42 },
	{ 0x21, 0xf0 }, { 0x1c, 0x0a }, { 0x1d, 0x08 }, { 0x1d, 0x0e },
};

static const U8 SensorInit[][2] =
{
	{ 0x12, 0x80 }, { 0x11, 0x01 }, { 0x11, 0x01 }, { 0x11, 0x01 },
	{ 0x11, 0x01 }, { 0x11, 0x01 }, { 0x11, 0x01 }, { 0x11, 0x01 },
	{ 0x11, 0x01 }, { 0x11, 0x01 }, { 0x11, 0x01 }, { 0x11, 0x01 },
	{ 0x3d, 0x03 }, { 0x17, 0x26 }, { 0x18, 0xa0 }, { 0x19, 0x07 },
	{ 0x1a, 0xf0 }, { 0x32, 0x00 }, { 0x29, 0xa0 }, { 0x2c, 0xf0 },
	{ 0x65, 0x20 }, { 0x11, 0x01 }, { 0x42, 0x7f }, { 0x63, 0xaa },
	{ 0x64, 0xff }, { 0x66, 0x00 }, { 0x13, 0xf0 }, { 0x0d, 0x41 },
	{ 0x0f, 0xc5 }, { 0x14, 0x11 }, { 0x22, 0x7f }, { 0x23, 0x03 },
	{ 0x24, 0x40 }, { 0x25, 0x30 }, { 0x26, 0xa1 }, { 0x2a, 0x00 },
	{ 0x2b, 0x00 }, { 0x6b, 0xaa }, { 0x13, 0xff }, { 0x90, 0x05 },
	{ 0x91, 0x01 }, { 0x92, 0x03 }, { 0x93, 0x00 }, { 0x94, 0x60 },
	{ 0x95, 0x3c }, { 0x96, 0x24 }, { 0x97, 0x1e }, { 0x98, 0x62 },
	{ 0x99, 0x80 }, { 0x9a, 0x1e }, { 0x9b, 0x08 }, { 0x9c, 0x20 },
	{ 0x9e, 0x81 }, { 0xa6, 0x07 }, { 0x7e, 0x0c }, { 0x7f, 0x16 },
	{ 0x80, 0x2a }, { 0x81, 0x4e }, { 0x82, 0x61 }, { 0x83, 0x6f },
	{ 0x84, 0x7b }, { 0x85, 0x86 }, { 0x86, 0x8e }, { 0x87, 0x97 },
	{ 0x88, 0xa4 }, { 0x89, 0xaf }, { 0x8a, 0xc5 }, { 0x8b, 0xd7 },
	{ 0x8c, 0xe8 }, { 0x8d, 0x20 }, { 0x2b, 0x00 }, { 0x22, 0x7f },
	{ 0x23, 0x03 }, { 0x11, 0x01 }, { 0x64, 0xff }, { 0x0d, 0x41 },
	{ 0x14, 0x41 }, { 0x0e, 0xcd }, { 0xac, 0xbf }, { 0x8e, 0x00 },
};

static const U8 BridgeVGA[][2] =
{
	{ 0x88, 0x00 }, { 0x1c, 0x00 }, { 0x1d, 0x40 }, { 0x1d, 0x02 },
	{ 0x1d, 0x00 }, { 0x1d, 0x02 }, { 0x1d, 0x58 }, { 0x1d, 0x00 },
	{ 0x8d, 0x1c }, { 0x8e, 0x80 }, { 0xc0, 0x50 }, { 0xc1, 0x3c },
	{ 0xc2, 0x0c }, { 0xc3, 0x69 },
};

static const U8 SensorVGA[][2] =
{
	{ 0x12, 0x00 }, { 0x17, 0x26 }, { 0x18, 0xa0 }, { 0x19, 0x07 },
	{ 0x1a, 0xf0 }, { 0x29, 0xa0 }, { 0x2c, 0xf0 }, { 0x65, 0x20 },
	{ 0x67, 0x00 },
};

static const U8 BridgeQVGA[][2] =
{
	{ 0x88, 0x00 }, { 0x1c, 0x00 }, { 0x1d, 0x40 }, { 0x1d, 0x02 },
	{ 0x1d, 0x00 }, { 0x1d, 0x01 }, { 0x1d, 0x4b }, { 0x1d, 0x00 },
	{ 0x8d, 0x1c }, { 0x8e, 0x80 }, { 0xc0, 0x28 }, { 0xc1, 0x1e },
	{ 0xc2, 0x0c }, { 0xc3, 0x69 },
};

static const U8 SensorQVGA[][2] =
{
	{ 0x12, 0x40 }, { 0x17, 0x3f }, { 0x18, 0x50 }, { 0x19, 0x03 },
	{ 0x1a, 0x78 }, { 0x29, 0x50 }, { 0x2c, 0x78 }, { 0x65, 0x2f },
	{ 0x67, 0x00 },
};

struct OV534_Mode
{
	U16 Width;
	U16 Height;
	U8 FPS;
	U8 SensorClock;
	U8 SensorPLL;
	U8 BridgeClock;
};

static const struct OV534_Mode Modes[] =
{
	{ 640, 480, 30, 0x04, 0x81, 0x02 },
	{ 640, 480, 15, 0x03, 0x41, 0x04 },
	{ 640, 480, 40, 0x02, 0xc1, 0x04 },
	{ 640, 480, 50, 0x01, 0x41, 0x02 },
	{ 640, 480, 60, 0x01, 0xc1, 0x04 },
	{ 320, 240, 30, 0x04, 0x41, 0x04 },
	{ 320, 240, 37, 0x03, 0x41, 0x04 },
	{ 320, 240, 50, 0x02, 0x41, 0x04 },
	{ 320, 240, 60, 0x04, 0xc1, 0x04 },
	{ 320, 240, 75, 0x03, 0xc1, 0x04 },
	{ 320, 240, 100, 0x02, 0xc1, 0x04 },
	{ 320, 240, 125, 0x02, 0x81, 0x02 },
	{ 320, 240, 137, 0x02, 0xc1, 0x02 },
	{ 320, 240, 150, 0x01, 0xc1, 0x04 },
	{ 320, 240, 187, 0x01, 0x81, 0x02 },
};

#define OV534_SEQUENCE(fn, unit, table) fn( unit, table, sizeof( table ) / sizeof( (table)[0] ) )

// --

static SEC_CODE S32
BridgeWrite( struct VideoUnit *unit, U8 reg, U8 value )
{
	return ( Video_Control_Index( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		1, 0, reg, & value, 1, 500000 ) );
}

// --

static SEC_CODE S32
BridgeRead( struct VideoUnit *unit, U8 reg, U8 *value )
{
	return ( Video_Control_Index( unit, REQTYPE_Read | REQTYPE_Vendor | REQTYPE_Device,
		1, 0, reg, value, 1, 500000 ) );
}

// --

static SEC_CODE S32
DelayUS( struct VideoUnit *unit, U32 microseconds )
{
	unit->Timer->Request.io_Command = TR_ADDREQUEST;
	unit->Timer->Time.Seconds = 0;
	unit->Timer->Time.Microseconds = microseconds;

	return ( DoIO( & unit->Timer->Request ) == 0 );
}

// --

static SEC_CODE S32
WaitSensor( struct VideoUnit *unit )
{
	U32 attempt;
	U8 status;

	for ( attempt = 0; attempt < 5; attempt++ )
	{
		if ( ( ! DelayUS( unit, 10000 ) ) || ( ! BridgeRead( unit, OV534_SENSOR_STATUS, & status ) ) )
		{
			break;
		}

		if ( status == 0 )
		{
			return ( TRUE );
		}

		if ( status == 4 )
		{
			break;
		}
	}

	return ( FALSE );
}

// --

static SEC_CODE S32
SensorWrite( struct VideoUnit *unit, U8 reg, U8 value )
{
	return ( ( BridgeWrite( unit, OV534_SENSOR_REGISTER, reg ) ) &&
		( BridgeWrite( unit, OV534_SENSOR_WRITE, value ) ) &&
		( BridgeWrite( unit, OV534_SENSOR_OPERATION, OV534_SENSOR_WRITE_REG ) ) &&
		( WaitSensor( unit ) ) );
}

// --

static SEC_CODE S32
SensorRead( struct VideoUnit *unit, U8 reg, U8 *value )
{
	return ( ( BridgeWrite( unit, OV534_SENSOR_REGISTER, reg ) ) &&
		( BridgeWrite( unit, OV534_SENSOR_OPERATION, OV534_SENSOR_SELECT_REG ) ) &&
		( WaitSensor( unit ) ) &&
		( BridgeWrite( unit, OV534_SENSOR_OPERATION, OV534_SENSOR_READ_REG ) ) &&
		( WaitSensor( unit ) ) &&
		( BridgeRead( unit, OV534_SENSOR_READ, value ) ) );
}

// --

static SEC_CODE S32
BridgeSequence( struct VideoUnit *unit, const U8 (*values)[2], U32 count )
{
	U32 index;

	for ( index = 0; index < count; index++ )
	{
		if ( ! BridgeWrite( unit, values[index][0], values[index][1] ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static SEC_CODE S32
SensorSequence( struct VideoUnit *unit, const U8 (*values)[2], U32 count )
{
	U32 index;

	for ( index = 0; index < count; index++ )
	{
		if ( ! SensorWrite( unit, values[index][0], values[index][1] ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static SEC_CODE S32
SetLED( struct VideoUnit *unit, S32 on )
{
	U8 direction;
	U8 output;

	if ( ( ! BridgeRead( unit, 0x21, & direction ) ) ||
		( ! BridgeWrite( unit, 0x21, direction | 0x80 ) ) ||
		( ! BridgeRead( unit, 0x23, & output ) ) ||
		( ! BridgeWrite( unit, 0x23, on ? ( output | 0x80 ) : ( output & 0x7f ) ) ) )
	{
		return ( FALSE );
	}

	return ( ( on ) || ( BridgeWrite( unit, 0x21, direction & 0x7f ) ) );
}

// --

static SEC_CODE S8
LastControlError( struct VideoUnit *unit )
{
	S8 error;

	error = unit->Control->IORequests[0]->io_Error;

	return ( error ? error : IOERR_OPENFAIL );
}

// --

SEC_CODE S8
Video_OV534_Stop( struct VideoUnit *unit )
{
	S8 error;

	error = 0;

	if ( ( ! unit->USBDetached ) && ( ! BridgeWrite( unit, 0xe0, 0x09 ) ) )
	{
		error = LastControlError( unit );
	}

	// Try to turn off the LED even if the bridge stop failed.
	if ( ( ! unit->USBDetached ) && ( ! SetLED( unit, FALSE ) ) && ( ! error ) )
	{
		error = LastControlError( unit );
	}

	if ( unit->USBDetached )
	{
		unit->OV534StopPending = FALSE;

		return ( USB2Err_Device_Detached );
	}

	if ( ! error )
	{
		unit->OV534StopPending = FALSE;
	}

	return ( error );
}

// --

SEC_CODE S8
Video_OV534_Init( struct VideoUnit *unit )
{
	S8 error;
	U8 high;
	U8 low;

	if ( unit->OV534Initialized )
	{
		return ( 0 );
	}

	unit->OV534StopPending = TRUE;

	if ( ( ! BridgeWrite( unit, 0xe7, 0x3a ) ) || ( ! BridgeWrite( unit, 0xe0, 0x08 ) ) ||
		( ! DelayUS( unit, 100000 ) ) ||
		( ! BridgeWrite( unit, OV534_SENSOR_ADDRESS, 0x42 ) ) ||
		( ! SensorWrite( unit, 0x12, 0x80 ) ) || ( ! DelayUS( unit, 10000 ) ) ||
		( ! SensorRead( unit, 0x0a, & high ) ) || ( ! SensorRead( unit, 0x0a, & high ) ) ||
		( ! SensorRead( unit, 0x0b, & low ) ) || ( ! SensorRead( unit, 0x0b, & low ) ) )
	{
		return ( LastControlError( unit ) );
	}

	if ( ( high != 0x77 ) || ( ( low & 0xf0 ) != 0x20 ) )
	{
		DebugPrintF( "USB Video: PlayStation Eye unsupported sensor %02lx%02lx\n", (U32) high, (U32) low );

		return ( IOERR_OPENFAIL );
	}

	if ( ( ! OV534_SEQUENCE( BridgeSequence, unit, BridgeInit ) ) || ( ! SetLED( unit, TRUE ) ) ||
		( ! OV534_SEQUENCE( SensorSequence, unit, SensorInit ) ) )
	{
		return ( LastControlError( unit ) );
	}

	error = Video_OV534_Stop( unit );

	if ( error )
	{
		return ( error );
	}

	unit->OV534Initialized = TRUE;

	DebugPrintF( "USB Video: PlayStation Eye OV534 sensor %02lx%02lx initialized\n", (U32) high, (U32) low );

	return ( 0 );
}

// --

SEC_CODE S32
Video_OV534_Mode( struct VideoUnit *unit, U32 number, struct VideoMode *mode )
{
	const struct OV534_Mode *entry;

	if ( number > sizeof( Modes ) / sizeof( Modes[0] ) )
	{
		return ( FALSE );
	}

	if ( ! number )
	{
		number = 1;
	}

	entry = & Modes[number - 1];
	mode->Format.Width = entry->Width;
	mode->Format.Height = entry->Height;
	mode->Format.Format = VIDEO_FORMAT_YUY2;
	mode->Format.MaxFrameSize = (U32) entry->Width * entry->Height * 2;
	mode->Format.FrameInterval = 10000000U / entry->FPS;
	mode->Format.VendorID = unit->StartMsg->VendorID;
	mode->Format.DeviceID = unit->StartMsg->DeviceID;
	mode->FrameIndex = number;
	mode->FormatIndex = 1;

	return ( TRUE );
}

// --

SEC_CODE S8
Video_OV534_Start( struct VideoUnit *unit )
{
	const struct OV534_Mode *mode;

	S32 configured;

	if ( ( ! unit->FrameIndex ) || ( unit->FrameIndex > sizeof( Modes ) / sizeof( Modes[0] ) ) )
	{
		return ( IOERR_OPENFAIL );
	}

	mode = & Modes[unit->FrameIndex - 1];
	unit->OV534StopPending = TRUE;

	if ( mode->Width == 640 )
	{
		configured = ( OV534_SEQUENCE( BridgeSequence, unit, BridgeVGA ) ) &&
			( OV534_SEQUENCE( SensorSequence, unit, SensorVGA ) );
	}
	else
	{
		configured = ( OV534_SEQUENCE( BridgeSequence, unit, BridgeQVGA ) ) &&
			( OV534_SEQUENCE( SensorSequence, unit, SensorQVGA ) );
	}

	if ( ( ! configured ) || ( ! SensorWrite( unit, 0x11, mode->SensorClock ) ) ||
		( ! SensorWrite( unit, 0x0d, mode->SensorPLL ) ) ||
		( ! BridgeWrite( unit, 0xe5, mode->BridgeClock ) ) ||
		( ! SetLED( unit, TRUE ) ) || ( ! BridgeWrite( unit, 0xe0, 0 ) ) )
	{
		return ( LastControlError( unit ) );
	}

	return ( 0 );
}

// --
