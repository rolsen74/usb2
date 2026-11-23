
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
Write_ASCII( FILE *file, const char *value )
{
	const U8 *data;

	data = (const U8 *) value;

	while( *data )
	{
		if ( ( *data >= 0x20 ) && ( *data <= 0x7e ) )
		{
			fputc( *data, file );
		}
		else
		{
			fprintf( file, "\\x%02X", (unsigned) *data );
		}

		data++;
	}
}

// --

struct DescriptorFile
{
	FILE *File;
	int Indent;
};

// --

static void
Write_Descriptor_Row( PTR target, U32 level, S32 heading, const char *field, const char *size,
	const char *value, const char *description )
{
	struct DescriptorFile *output;

	int indent;

	output = target;
	indent = output->Indent + (int) level * 2;

	if ( heading )
	{
		fprintf( output->File, "%*s%s\n", indent, "", field );
	}
	else
	{
		fprintf( output->File, "%*s%-24s  %4s  %-12s", indent, "", field, size, value );

		if ( description[0] )
		{
			fputs( "  ", output->File );

			Write_ASCII( output->File, description );
		}

		fputc( '\n', output->File );
	}
}

// --

static void
Write_Function( FILE *file, int indent, const struct USB2_FktNode *node )
{
	const struct USB2_Device_Desc *device;
	struct DescriptorFile output;

	device = & node->DevDsc;

	if ( node->Port )
	{
		fprintf( file, "%*sPort %u: ", indent, "", (unsigned) node->Port );
	}
	else
	{
		fprintf( file, "%*s", indent, "" );
	}

	Write_ASCII( file, node->ProductStr[0] ? node->ProductStr :
		( device->DeviceClass == USBCLASS_HUB ? ( node->Port ? "USB hub" : "Root hub" ) : "USB device" ) );

	fprintf( file, " [%04X:%04X]\n", (unsigned) LE_SWAP16( device->VendorID ), (unsigned) LE_SWAP16( device->DeviceID ) );

	output.File = file;
	output.Indent = indent + 2;

	fprintf( file, "%*s%-26s  %4s  %-12s  %s\n", output.Indent, "", "Field", "Size", "Value", "Description" );

	Describe_Descriptors( node, Write_Descriptor_Row, & output );
}

// --

static void
Write_Tree_Node( FILE *file, int indent, struct USB2_FktNode *node )
{
	struct USB2_FktNode *child;

	U32 port;

	if ( node->Type == TYPE_HCD )
	{
		fprintf( file, "%*sHost controller: ", indent, "" );

		Write_ASCII( file, node->Name ? node->Name : "Unknown" );

		fputc( '\n', file );
	}
	else if ( node->Type == TYPE_FUNCTION )
	{
		Write_Function( file, indent, node );
	}
	else
	{
		// Interface nodes are included in the configuration stream.
		return;
	}

	if ( node->Functions )
	{
		for( port = 0; port < node->Ports; port++ )
		{
			child = node->Functions[port];

			if ( child )
			{
				Write_Tree_Node( file, indent + 2, child );
			}
			else
			{
				fprintf( file, "%*sPort %u: Empty\n", indent + 2, "", (unsigned) ( port + 1 ) );
			}
		}
	}
	else
	{
		for( child = (PTR) GetHead( & node->Header ); child; child = (PTR) GetSucc( & child->Node ) )
		{
			Write_Tree_Node( file, indent + 2, child );
		}
	}
}

// --

S32
Save_Device_Tree( const char *path )
{
	struct USB2_FktNode *node;

	FILE *file;
	S32 written;

	file = fopen( path, "w" );

	if ( ! file )
	{
		return ( FALSE );
	}

	Handle_Notify();

	fputs( VERS " USB tree\nDescriptors exposed by usb2.device (cached configuration index 0)\n\n", file );

	node = (PTR) GetHead( & DeviceTreeUSBList );

	if ( ! node )
	{
		fputs( "No USB host controllers found.\n", file );
	}

	for( ; node; node = (PTR) GetSucc( & node->Node ) )
	{
		Write_Tree_Node( file, 0, node );

		fputc( '\n', file );
	}

	written = ! ferror( file );

	if ( fclose( file ) != 0 )
	{
		written = FALSE;
	}

	return ( written );
}

// --

void
Request_Device_Tree_Save( struct Window *window )
{
	struct FileRequester *request;
	struct EasyStruct error;

	char path[1024];
	const char *message;

	message = NULL;

	request = AllocAslRequestTags( ASL_FileRequest, TAG_END );

	if ( ! request )
	{
		message = "Could not open the save requester.";
	}
	else
	{
		if ( AslRequestTags( request,
			ASLFR_Window, window,
			ASLFR_SleepWindow, TRUE,
			ASLFR_TitleText, "Save USB tree",
			ASLFR_DoSaveMode, TRUE,
			ASLFR_InitialFile, "USB2Tree.txt",
			TAG_END ) )
		{
			if ( strlen( request->fr_Drawer ) < sizeof( path ) )
			{
				strcpy( path, request->fr_Drawer );

				if ( ( ! request->fr_File[0] ) || ( ! AddPart( path, request->fr_File, sizeof( path ) ) ) )
				{
					message = "Could not use the selected file name.";
				}
				else if ( ! Save_Device_Tree( path ) )
				{
					message = "Could not write the USB tree to the selected file.";
				}
			}
			else
			{
				message = "The selected path is too long.";
			}
		}

		FreeAslRequest( request );
	}

	if ( message )
	{
		error.es_StructSize = sizeof( error );
		error.es_Flags = 0;
		error.es_Title = "USB2Analyzer";
		error.es_TextFormat = message;
		error.es_GadgetFormat = "OK";

		EasyRequestArgs( window, & error, NULL, NULL );
	}
}

// --
