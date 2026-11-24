
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include <interfaces/usb2smartcard.h>
#include <stdio.h>

// --

static int
Probe( struct USB2SmartCardIFace *card, ULONG unit, ULONG slot, BOOL atr )
{
	struct USB2SmartCardReply reply;
	struct USB2SmartCardInfo info;

	LONG result;
	ULONG index;
	ULONG instance;
	UBYTE bytes[USB2SMARTCARD_ATR_MAX];
	static const char * const states[] = { "active", "present, inactive", "no card", "unknown" };

	result = card->GetInstance( unit, &info, &instance );

	if ( result != USB2SC_Okay )
	{
		printf( "Unit %lu is unavailable (%ld).\n", unit, result );

		return ( RETURN_WARN );
	}

	result = card->CommandInstance( unit, instance, slot, USB2SCC_Status, NULL, 0, NULL, 0, & reply );

	if ( result != USB2SC_Okay )
	{
		printf( "Unit %lu slot %lu status failed (%ld, reader error $%02X).\n",
			unit, slot, result, (unsigned) reply.ReaderError );

		return ( RETURN_WARN );
	}

	printf( "Unit %lu slot %lu: %s.\n", unit, slot, states[reply.State] );

	if ( ( ! atr ) || ( reply.State == USB2SC_NoCard ) )
	{
		return ( RETURN_OK );
	}

	result = card->CommandInstance( unit, instance, slot, USB2SCC_PowerOn, NULL, 0, bytes, sizeof( bytes ), & reply );

	if ( result != USB2SC_Okay )
	{
		printf( "Power-on failed (%ld, reader error $%02X).\n", result, (unsigned) reply.ReaderError );

		return ( RETURN_WARN );
	}

	printf( "ATR:" );

	for( index = 0; index < reply.Length; index++ )
	{
		printf( " %02X", (unsigned) bytes[index] );
	}

	puts( "" );

	return ( RETURN_OK );
}

// --

int
main( void )
{
	struct RDArgs *args;
	struct Library *driver;
	struct USB2SmartCardIFace *card;
	struct USB2SmartCardInfo info;

	LONG values[3];
	LONG unit;
	LONG slot;
	ULONG found;
	int result;
	int status;

	values[0] = 0;
	values[1] = 0;
	values[2] = 0;
	found = 0;
	result = RETURN_OK;

	args = IDOS->ReadArgs( "UNIT/N,SLOT/N,ATR/S", values, NULL );

	if ( ! args )
	{
		IDOS->PrintFault( IDOS->IoErr(), "USB2SmartCardTest" );

		return ( RETURN_ERROR );
	}

	unit = values[0] ? *(LONG *) values[0] : -1;
	slot = values[1] ? *(LONG *) values[1] : 0;

	if ( ( ( values[0] ) && ( ( unit < 0 ) || ( unit >= USB2SMARTCARD_UNITS ) ) ) ||
		( ( values[1] ) && ( ( unit < 0 ) || ( slot < 0 ) || ( slot > 255 ) ) ) ||
		( ( values[2] ) && ( unit < 0 ) ) )
	{
		puts( "Choose UNIT (0-31) before using SLOT (0-255) or ATR." );
		result = RETURN_ERROR;
		goto done;
	}

	driver = IExec->OpenLibrary( USB2SMARTCARD_DRIVER, 1 );

	if ( ! driver )
	{
		puts( "No USB CCID driver is active. With cls_SmartCard.Class and cls_SmartCard.Driver installed, reconnect the reader." );
		result = RETURN_WARN;
		goto done;
	}

	card = (APTR) IExec->GetInterface( driver, USB2SMARTCARD_INSTANCE_INTERFACE, USB2SMARTCARD_INSTANCE_VERSION, NULL );

	if ( card )
	{
		if ( unit >= 0 )
		{
			result = Probe( card, unit, slot, values[2] != 0 );
		}
		else
		{
			for( unit = 0; unit < USB2SMARTCARD_UNITS; unit++ )
			{
				status = card->GetInfo( unit, & info );

				if ( status == USB2SC_Okay )
				{
					printf( "Unit %ld USB %04X:%04X, %u slot(s), features $%08lX.\n", unit,
						(unsigned) info.Vendor, (unsigned) info.Product, (unsigned) info.Slots, info.Features );
					found++;
				}
			}

			if ( ! found )
			{
				puts( "No USB CCID readers found." );
				result = RETURN_WARN;
			}
		}

		IExec->DropInterface( (APTR) card );
	}
	else
	{
		puts( "cls_SmartCard.Driver does not expose the CCID version 2 interface." );
		result = RETURN_ERROR;
	}

	IExec->CloseLibrary( driver );

done:
	IDOS->FreeArgs( args );

	return ( result );
}

// --
