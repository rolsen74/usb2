
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"
#include "HIDEvents.h"

// --

struct BTInputDevice
{
	struct BTHIDEvents Events;
	struct MsgPort *Port;
	struct IOStdReq *Request;
	LONG Error;
	uint8_t PreviousCode[2];
	uint8_t PreviousQual[2];
};

// --

static int32_t
SendInputEvent( void *context, uint8_t kind, uint16_t code, uint16_t qualifier, int16_t x, int16_t y )
{
	struct BTInputDevice *input;
	struct InputEvent event;

	LONG status;

	input = context;

	memset( & event, 0, sizeof( event ) );

	event.ie_Class = kind;
	event.ie_Code = code;
	event.ie_Qualifier = qualifier;

	if ( kind == IECLASS_RAWKEY )
	{
		event.ie_Prev1DownCode = input->PreviousCode[0];
		event.ie_Prev1DownQual = input->PreviousQual[0];
		event.ie_Prev2DownCode = input->PreviousCode[1];
		event.ie_Prev2DownQual = input->PreviousQual[1];
	}
	else
	{
		event.ie_X = x;
		event.ie_Y = y;
	}

	input->Request->io_Command = IND_ADDEVENT;
	input->Request->io_Data = & event;
	input->Request->io_Length = sizeof( event );

	status = DoIO( (APTR) input->Request );

	if ( status )
	{
		input->Error = status;

		return ( FALSE );
	}

	if ( ( kind == IECLASS_RAWKEY ) && ( ! ( code & IECODE_UP_PREFIX ) ) &&
		( ( code < 0x60 ) || ( code > 0x67 ) ) )
	{
		input->PreviousCode[1] = input->PreviousCode[0];
		input->PreviousCode[0] = code;
		input->PreviousQual[1] = input->PreviousQual[0];
		input->PreviousQual[0] = qualifier;
	}

	return ( TRUE );
}

// --

static int32_t
StartInput( void *context )
{
	struct BTInputDevice *input;

	input = context;

	// The USB worker owns the reply port and runs the callbacks.
	input->Port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! input->Port )
	{
		return ( FALSE );
	}

	input->Request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *input->Request ),
		ASOIOR_ReplyPort, input->Port, TAG_END );

	if ( input->Request )
	{
		input->Error = OpenDevice( "input.device", 0, (APTR) input->Request, 0 );

		if ( ! input->Error )
		{
			input->Events.Context = input;
			input->Events.Emit = SendInputEvent;
			input->Events.NumLock = TRUE;

			return ( TRUE );
		}

		FreeSysObject( ASOT_IOREQUEST, input->Request );
	}

	FreeSysObject( ASOT_PORT, input->Port );

	return ( FALSE );
}

// --

static int32_t
HandleInput( void *context, const struct BTHIDReport *report )
{
	struct BTInputDevice *input;

	input = context;

	return ( ApplyHIDReport( & input->Events, report ) );
}

// --

static void
StopInput( void *context )
{
	struct BTInputDevice *input;

	input = context;

	ApplyHIDReport( & input->Events, NULL );

	CloseDevice( (APTR) input->Request );

	FreeSysObject( ASOT_IOREQUEST, input->Request );

	FreeSysObject( ASOT_PORT, input->Port );
}

// --

LONG
ConnectBluetoothInput( struct IORequest *request, ULONG unit, const char *address )
{
	struct Bluetooth2IFace *interface;
	struct BTInputDevice input;
	struct BTHIDInput hid;
	struct BTHCIError error;
	enum BTHCIResult result;

	memset( & hid, 0, sizeof( hid ) );

	memset( & input, 0, sizeof( input ) );

	if ( ! ParseAddress( address, hid.Monitor.Address ) )
	{
		Printf( "HID requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	hid.Context = & input;
	hid.Start = StartInput;
	hid.Report = HandleInput;
	hid.Stop = StopInput;

	Printf( "Connecting %s on unit %lu for system input. Ctrl-C stops.\n", address, unit );

	result = interface->hidinput_ConnectController( unit, & hid, & error );

	DropInterface( (APTR) interface );

	switch( result )
	{
		case BT_HCI_OK:
		case BT_RFCOMM_CANCELLED:
		case BT_HID_DISCONNECTED:
			Printf( "Bluetooth input ended after %lu reports.\n", (ULONG) hid.Monitor.Count );

			return ( RETURN_OK );

		case BT_HCI_NO_SAVED_KEY:
			Printf( "Use BOND for this device and controller before enabling system input.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller is available on unit %lu.\n", unit );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "INPUT requires cls_Bluetooth.Driver 1.18 and a Classic Basic Mode L2CAP connection.\n" );

			break;

		case BT_HID_REJECTED:
			Printf( "The device rejected Boot Protocol with handshake $%02lx.\n", (ULONG) hid.Monitor.Handshake );

			break;

		case BT_HID_INPUT_ERROR:
			Printf( "Could not deliver Bluetooth input through input.device (error %ld).\n", (LONG) input.Error );

			break;

		default:
			ReportHCIError( result, & error );

			break;
	}

	return ( RETURN_FAIL );
}

// --
