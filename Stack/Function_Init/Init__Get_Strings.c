
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#define USB_INIT_STRING_BUDGET_US 500000
#define USB_INIT_STRING_MAX 255

struct InitStringReads
{
	struct RealFunctionNode *Function;
	struct USB2_IORequest *Request;
	struct USB2_SetupData *Setup;
	U16 *Cache[USB_INIT_STRING_MAX + 1];
	U8 Tried[USB_INIT_STRING_MAX + 1];
	U8 *Buffer;
	U64 Deadline;
	S32 Cancelled;
};

// --

SEC_CODE static U64
InitUptimeUS( struct USBBase *usbbase )
{
	struct TimeVal time;

	usbbase->usb_ITimer->GetUpTime( & time );

	return ( (U64) time.Seconds * 1000000 + time.Microseconds );
}

// --

SEC_CODE static S32
ReadInitString( struct USBBase *usbbase, struct InitStringReads *reads, U32 id )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *sd;

	U64 now;
	U32 length;

	if ( reads->Cancelled )
	{
		return ( FALSE );
	}

	reads->Cancelled = InitCancelled( usbbase, reads->Function, USB2Err_NoError );

	now = InitUptimeUS( usbbase );

	if ( ( reads->Cancelled ) || ( now >= reads->Deadline ) )
	{
		return ( FALSE );
	}

	ioreq = reads->Request;
	sd = reads->Setup;
	sd->RequestType = REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device;
	sd->RequestCode = REQCODE_Get_Descriptor;
	sd->Value = LE_SWAP16( ( DSCTYPE_String << 8 ) | id );
	sd->Index = LE_SWAP16( ( id ) ? reads->Function->fkt_LanguageID : 0 );
	sd->Length = LE_SWAP16( USB_INIT_STRING_MAX );
	ioreq->io_Command = CMD_READ;
	ioreq->io_SetupData = sd;
	ioreq->io_SetupLength = sizeof( *sd );
	ioreq->io_Data = reads->Buffer;
	ioreq->io_Length = USB_INIT_STRING_MAX;
	ioreq->io_TimeOut = (U32) ( reads->Deadline - now );

	IO_DO( ioreq );

	reads->Cancelled = InitCancelled( usbbase, reads->Function, ioreq->io_Error );

	if ( ( reads->Cancelled ) || ( ioreq->io_Error ) || ( ioreq->io_Actual < 2 ) )
	{
		USBDEBUG( "String descriptor #%lu: error %ld, %lu bytes", id, (S32) ioreq->io_Error, ioreq->io_Actual );

		return ( FALSE );
	}

	length = reads->Buffer[0];

	return ( ( reads->Buffer[1] == DSCTYPE_String ) && ( length >= 2 ) &&
		( ! ( length & 1 ) ) && ( length <= ioreq->io_Actual ) );
}

// --

SEC_CODE static U16
ReadInitLanguage( struct USBBase *usbbase, struct InitStringReads *reads )
{
	U16 first;
	U16 language;
	U32 offset;

	if ( ! ReadInitString( usbbase, reads, 0 ) )
	{
		return ( 0 );
	}

	first = 0;

	for ( offset = 2 ; offset < reads->Buffer[0] ; offset += 2 )
	{
		language = reads->Buffer[offset] | ( reads->Buffer[offset + 1] << 8 );

		if ( language == 0x0409 )
		{
			return ( language );
		}

		if ( ! first )
		{
			first = language;
		}
	}

	return ( first );
}

// --

SEC_CODE static S32
CacheInitString( struct USBBase *usbbase, struct InitStringReads *reads, U32 id, U16 **result, S32 *length )
{
	U16 *copy;
	U8 *data;
	U32 size;

	if ( ( ! id ) || ( reads->Cancelled ) )
	{
		return ( FALSE );
	}

	if ( reads->Tried[id] )
	{
		data = (PTR) reads->Cache[id];

		if ( ! data )
		{
			return ( FALSE );
		}
	}
	else
	{
		reads->Tried[id] = TRUE;

		if ( ! ReadInitString( usbbase, reads, id ) )
		{
			return ( FALSE );
		}

		data = reads->Buffer;
	}

	size = data[0];

	copy = MEM_ALLOCVEC( size + 1, TRUE );

	if ( ! copy )
	{
		return ( FALSE );
	}

	MEM_COPY( data, copy, size );

	// The first destination owns the cached buffer until enumeration ends.
	if ( ! reads->Cache[id] )
	{
		reads->Cache[id] = copy;
	}

	*result = copy;
	*length = size;

	return ( TRUE );
}

// --

SEC_CODE static S32
InitHasStrings( struct RealFunctionNode *fn )
{
	struct USB2_ConfigNode *cn;
	struct USB2_InterfaceGroup *ig;
	struct USB2_InterfaceHeader *ih;
	struct USB2_InterfaceNode *in;

	if ( ( fn->fkt_DeviceDescriptor->ManufacturerStrNr ) || ( fn->fkt_DeviceDescriptor->DeviceStrNr ) ||
		( fn->fkt_DeviceDescriptor->SerialStrNr ) )
	{
		return ( TRUE );
	}

	for ( cn = fn->fkt_Configs.uh_Head; cn; cn = Node_Next( cn ) )
	{
		if ( cn->cfg_Descriptor->ConfigurationStrNr )
		{
			return ( TRUE );
		}

		for ( ig = cn->cfg_InterfaceGroups.uh_Head; ig; ig = Node_Next( ig ) )
		{
			for ( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next( ih ) )
			{
				for ( in = ih->ih_AltSettings.uh_Head; in; in = Node_Next( in ) )
				{
					if ( in->in_Descriptor->InterfaceStrNr )
					{
						return ( TRUE );
					}
				}
			}
		}
	}

	return ( FALSE );
}

// --

SEC_CODE static S32
__Get_Strings( struct RealFunctionNode *fn, struct USB2_IORequest *ioreq, struct USB2_SetupData *sd,
	struct USBBase *usbbase )
{
	struct USB2_InterfaceHeader *ih;
	struct USB2_InterfaceGroup *ig;
	struct USB2_InterfaceNode *in;
	struct USB2_ConfigNode *cn;
	struct InitStringReads reads;

	U32 timeout;

	if ( InitCancelled( usbbase, fn, USB2Err_NoError ) )
	{
		return ( FALSE );
	}

	if ( ! InitHasStrings( fn ) )
	{
		return ( TRUE );
	}

	MEM_SET( & reads, 0, sizeof( reads ) );

	reads.Buffer = MEM_ALLOCIOBUFFER( USB_INIT_STRING_MAX + 1, FALSE );

	if ( ! reads.Buffer )
	{
		return ( TRUE );
	}

	timeout = ioreq->io_TimeOut;
	reads.Function = fn;
	reads.Request = ioreq;
	reads.Setup = sd;

	reads.Deadline = InitUptimeUS( usbbase ) + USB_INIT_STRING_BUDGET_US;
	fn->fkt_LanguageID = ReadInitLanguage( usbbase, & reads );

	if ( fn->fkt_LanguageID )
	{
		CacheInitString( usbbase, & reads, fn->fkt_DeviceDescriptor->ManufacturerStrNr,
			& fn->fkt_ManufacturerStr, & fn->fkt_ManufacturerStrLen );

		CacheInitString( usbbase, & reads, fn->fkt_DeviceDescriptor->DeviceStrNr,
			& fn->fkt_DeviceStr, & fn->fkt_DeviceStrLen );

		CacheInitString( usbbase, & reads, fn->fkt_DeviceDescriptor->SerialStrNr,
			& fn->fkt_SerialStr, & fn->fkt_SerialStrLen );

		for ( cn = fn->fkt_Configs.uh_Head ; ( cn ) && ( ! reads.Cancelled ) ; cn = Node_Next( cn ) )
		{
			if ( CacheInitString( usbbase, & reads, cn->cfg_Descriptor->ConfigurationStrNr,
				& cn->cfg_String, & cn->cfg_StringLen ) )
			{
				cn->cfg_StringID = MISC_NEWNOTIFYID();
			}

			for ( ig = cn->cfg_InterfaceGroups.uh_Head ; ( ig ) && ( ! reads.Cancelled ) ; ig = Node_Next( ig ) )
			{
				for ( ih = ig->ig_Group.uh_Head ; ( ih ) && ( ! reads.Cancelled ) ; ih = Node_Next( ih ) )
				{
					for ( in = ih->ih_AltSettings.uh_Head ; ( in ) && ( ! reads.Cancelled ) ; in = Node_Next( in ) )
					{
						if ( CacheInitString( usbbase, & reads, in->in_Descriptor->InterfaceStrNr,
							& in->in_String, & in->in_StringLen ) )
						{
							in->in_StringID = MISC_NEWNOTIFYID();
						}
					}
				}
			}
		}
	}

	ioreq->io_TimeOut = timeout;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	MEM_FREEIOBUFFER( reads.Buffer );

	return ( ! reads.Cancelled );
}

// --
