
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"
#include "Playback.h"

// --

#define RTL2832_VENDOR	0x0bda
#define RTL2832_PRODUCT 0x2838

struct USB2IFace * IUSB2;

// --

static S32
ProbeReceiver ( U32 frequency, U8 * samples, U32 bytes, S32 listen, U32 output )
{
	struct USB2_Interface * ifc;
	struct USB2_Register *	reg;
	struct Message *		msg;

	U8 * config;
	U8	 endpoint;
	U8	 tunerAddress;
	U32	 size;
	U32	 index;
	S32	 result;

	for ( index = 0;; index++ )
	{
		ifc = USB2_Ifc_FindTags ( USB2Tag_Find_VendorID, RTL2832_VENDOR, USB2Tag_Find_DeviceID, RTL2832_PRODUCT,
								  USB2Tag_Find_Class, USBCLASS_Vendor_Specific, USB2Tag_Find_Index, index, TAG_END );

		if ( ! ifc )
		{
			Printf ( "No RTL2832U receiver (0BDA:2838) with interface 0 found.\n" );

			return ( RETURN_FAIL );
		}

		if ( ifc->IFC_Num == 0 )
		{
			break;
		}
	}

	reg = USB2_Reg_RegisterTags ( USB2Tag_Reg_Interface, ifc, USB2Tag_Reg_Title, "USB2SDRTest", TAG_END );

	if ( ! reg )
	{
		Printf ( "Could not claim RTL2832U interface 0. It may already be in use.\n" );

		return ( RETURN_FAIL );
	}

	config = NULL;
	result = RETURN_FAIL;

	if ( ( USB2_Attr_Get ( reg->FKT_ID, USB2Tag_Attr_ConfigSize, &size, sizeof ( size ) ) != sizeof ( size ) )
		 || ( size < sizeof ( struct USB2_Config_Desc ) ) || ( size > 65535 ) )
	{
		Printf ( "Could not read the RTL2832U configuration size.\n" );

		goto bailout;
	}

	config = AllocVecTags ( size, TAG_END );

	if ( ! config )
	{
		Printf ( "Could not allocate the USB descriptor buffer.\n" );

		goto bailout;
	}

	if ( USB2_Attr_Get ( reg->FKT_ID, USB2Tag_Attr_ConfigDesc, config, size ) != size )
	{
		Printf ( "Could not read the RTL2832U configuration descriptors.\n" );

		goto bailout;
	}

	endpoint = SDR_FindBulkIn ( config, size, 0 );

	if ( ! endpoint )
	{
		Printf ( "RTL2832U interface 0 alternate 0 has no usable Bulk IN endpoint.\n" );

		goto bailout;
	}

	Printf ( "RTL2832U 0BDA:2838, interface 0, Bulk IN $%02lx\n", (U32)endpoint );

	result = SDR_ProbeTuner ( reg->Res_Control, &tunerAddress );

	if ( frequency )
	{
		if ( ( result == RETURN_OK ) && ( tunerAddress == SDR_R820T_ADDRESS ) )
		{
			if ( listen )
			{
				result = SDR_ListenFM ( reg, endpoint, frequency, output );
			}
			else
			{
				result = SDR_CaptureIQ ( reg, endpoint, frequency, samples, bytes );
			}
		}
		else
		{
			if ( result == RETURN_OK )
			{
				Printf ( "I/Q capture currently supports the R820T-family tuner.\n" );
			}

			result = RETURN_FAIL;
		}
	}

bailout:
	FreeVec ( config );

	while ( ( msg = GetMsg ( reg->Stack_MsgPort ) ) )
	{
		ReplyMsg ( msg );
	}

	USB2_Reg_Unregister ( reg );

	return ( result );
}

// --

int
main ( int argc, char ** argv )
{
	struct RDArgs *	   args;
	struct MsgPort *   port;
	struct IORequest * request;

	const char * path;
	U8 *		 samples;
	LONG		 options[7];
	U32			 frequency;
	U32			 seconds;
	U32			 bytes;
	U32			 output;
	S32			 listen;
	S32			 saveWav;
	S32			 result;
	S32			 error;

	(void)argv;
	args	   = NULL;
	request	   = NULL;
	port	   = NULL;
	options[0] = 0;
	options[1] = 0;
	options[2] = 0;
	options[3] = 0;
	options[4] = 0;
	options[5] = 0;
	options[6] = 0;
	frequency  = 0;
	seconds	   = 1;
	bytes	   = 0;
	samples	   = NULL;
	output	   = SDR_OUTPUT_AUTO;
	listen	   = FALSE;
	saveWav	   = FALSE;
	path	   = "RAM:usb2sdr.iq";
	result	   = RETURN_FAIL;

	if ( argc )
	{
		args = ReadArgs ( "CAPTURE/K/N,FILE/K,DECODE/K,WAV/K,SECONDS/K/N,LISTEN/K/N,OUTPUT/K/N", options, NULL );

		if ( ! args )
		{
			PrintFault ( IoErr (), "USB2SDRTest" );

			goto bailout;
		}
	}

	if ( options[2] )
	{
		if ( ( options[0] ) || ( options[1] ) || ( options[4] ) || ( options[5] ) || ( options[6] ) )
		{
			Printf ( "Use DECODE with the input file and an optional WAV path.\n" );

			goto bailout;
		}

		path = options[3] ? (const char *)options[3] : "RAM:usb2sdr.wav";

		result = SDR_DecodeFM ( (const char *)options[2], path );

		goto bailout;
	}

	if ( options[5] )
	{
		if ( ( options[0] ) || ( options[1] ) || ( options[3] ) || ( options[4] ) )
		{
			Printf ( "Use LISTEN with a frequency in Hz and an optional OUTPUT unit.\n" );

			goto bailout;
		}

		listen = TRUE;
	}

	if ( options[6] )
	{
		output = *(LONG *)options[6];

		if ( ( ! listen ) || ( output < AUDIO_PLAYBACK_UNIT_BASE ) || ( output >= AUDIO_MAX_UNITS ) )
		{
			Printf ( "OUTPUT selects a USB audio playback unit from 16 to 31 for LISTEN.\n" );

			goto bailout;
		}
	}

	if ( ( ! options[0] ) && ( ( options[1] ) || ( options[3] ) || ( options[4] ) ) )
	{
		Printf ( "Use CAPTURE with a frequency in Hz before specifying FILE, WAV or SECONDS.\n" );

		goto bailout;
	}

	if ( ( options[1] ) && ( options[3] ) )
	{
		Printf ( "Choose FILE for raw I/Q or WAV for FM audio.\n" );

		goto bailout;
	}

	if ( ( options[0] ) || ( listen ) )
	{
		frequency = *(LONG *)( listen ? options[5] : options[0] );

		if ( ( frequency < SDR_FM_MIN_HZ ) || ( frequency > SDR_FM_MAX_HZ ) )
		{
			Printf ( "%s expects a frequency in Hz from 87500000 to 108000000.\n", listen ? "LISTEN" : "CAPTURE" );

			goto bailout;
		}
	}

	if ( options[4] )
	{
		seconds = *(LONG *)options[4];

		if ( ( ! seconds ) || ( seconds > SDR_MAX_CAPTURE_SECONDS ) )
		{
			Printf ( "SECONDS must be between 1 and %lu.\n", (U32)SDR_MAX_CAPTURE_SECONDS );

			goto bailout;
		}
	}

	if ( options[1] )
	{
		path = (const char *)options[1];
	}
	else if ( options[3] )
	{
		path = (const char *)options[3];
		saveWav = TRUE;
	}

	if ( options[0] )
	{
		bytes = seconds * SDR_IQ_BYTES_PER_SECOND;

		samples = AllocVecTags ( bytes, TAG_END );

		if ( ! samples )
		{
			Printf ( "Could not allocate the I/Q capture buffer.\n" );

			goto bailout;
		}
	}

	port = AllocSysObjectTags ( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		goto bailout;
	}

	request = AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( *request ), ASOIOR_ReplyPort, port, TAG_END );

	if ( ! request )
	{
		goto bailout;
	}

	error = OpenDevice ( "usb2.device", 0, request, 0 );

	if ( error )
	{
		Printf ( "Could not open usb2.device (error %ld).\n", error );

		goto bailout;
	}

	IUSB2 = (PTR)GetInterface ( (PTR)request->io_Device, "main", 1, NULL );

	if ( IUSB2 )
	{
		result = ProbeReceiver ( frequency, samples, bytes, listen, output );

		DropInterface ( (PTR)IUSB2 );
	}
	else
	{
		Printf ( "Could not open the usb2.device main interface.\n" );
	}

	CloseDevice ( request );

	if ( ( samples ) && ( ( result == RETURN_OK ) || ( result == RETURN_WARN ) ) )
	{
		if ( saveWav )
		{
			Printf ( "Decoding captured I/Q to mono FM audio...\n" );

			error = ! SDR_SaveFM ( path, samples, bytes );
		}
		else
		{
			error = ! SDR_SaveIQ ( path, samples, bytes );
		}

		if ( error )
		{
			result = RETURN_FAIL;
		}
	}

bailout:
	FreeVec ( samples );

	FreeSysObject ( ASOT_IOREQUEST, request );

	FreeSysObject ( ASOT_PORT, port );

	if ( args )
	{
		FreeArgs ( args );
	}

	return ( result );
}

// --
