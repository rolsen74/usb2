
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <stdlib.h>

// --

#define IO_CAPTURE_ROWS 256

const char *const IOCaptureTitles[IO_CAPTURE_COLUMNS] = { "#", "Request", "Device", "Endpoint", "Bytes", "Result" };
const char *const ActiveIOTitles[IO_CAPTURE_COLUMNS] = { "HCD", "Request", "Device", "Endpoint", "Bytes", "State" };
static const char *const EndpointTypes[] = { "Control", "Isochronous", "Bulk", "Interrupt" };
static U32 CaptureCount;
static U32 ActiveSortColumn;
static S32 ActiveSortReverse;

// --

const struct USB2_NotifyMessage *
Get_IO_Capture( struct Node *row )
{
	const struct USB2_NotifyMessage *entry;

	entry = NULL;

	if ( row )
	{
		GetListBrowserNodeAttrs( row, LBNA_UserData, &entry, TAG_END );
	}

	return ( entry );
}

// --

static void
Free_Capture_Row( struct Node *row )
{
	const struct USB2_NotifyMessage *entry;

	entry = Get_IO_Capture( row );

	FreeListBrowserNode( row );

	FreeVec( (PTR) entry );
}

// --

void
Free_IO_Request_Rows( struct List *list )
{
	struct Node *row;

	while( ( row = RemHead( list ) ) )
	{
		Free_Capture_Row( row );
	}
}

// --

void
Free_IO_Capture( struct List *list )
{
	Free_IO_Request_Rows( list );

	CaptureCount = 0;
}

// --

static const char *
Capture_Request_Name( U16 command, STR text, U32 size )
{
	switch( command )
	{
		case CMD_READ: return ( "Read" );
		case CMD_WRITE: return ( "Write" );
	}

	snprintf( text, size, "0x%04X", (unsigned) command );

	return ( text );
}

// --

static const char *
Capture_Endpoint_Type( U8 type )
{
	return ( type < sizeof( EndpointTypes ) / sizeof( EndpointTypes[0] ) ? EndpointTypes[type] : "Unknown" );
}

// --

static const char *
Active_IO_State( U16 state )
{
	switch( state )
	{
		case USB2IO_Pending: return ( "Pending" );
		case USB2IO_Queued: return ( "Queued" );
		case USB2IO_Processing: return ( "Processing" );
		case USB2IO_Active: return ( "Active" );
		default: return ( "Unknown" );
	}
}

// --

static U32
Capture_Text_Width( struct RastPort *rp, const char *text )
{
	return ( TextLength( rp, text, strlen( text ) ) + 16 );
}

// --

void
Measure_IO_Capture( struct List *list, struct RastPort *rp, const char *const *titles, U32 widths[IO_CAPTURE_COLUMNS] )
{
	struct Node *row;

	const char *text;
	U32 column;
	U32 width;

	for( column = 0; column < IO_CAPTURE_COLUMNS; column++ )
	{
		widths[column] = Capture_Text_Width( rp, titles[column] );
	}

	for( row = GetHead( list ); row; row = GetSucc( row ) )
	{
		for( column = 0; column < IO_CAPTURE_COLUMNS; column++ )
		{
			text = NULL;

			GetListBrowserNodeAttrs( row, LBNA_Column, column, LBNCA_Text, &text, TAG_END );

			if ( text )
			{
				width = Capture_Text_Width( rp, text );
				widths[column] = MAX( widths[column], width );
			}
		}
	}
}

// --

static struct Node *
Build_Capture_Row( USB2_ID id, const struct USB2_IORequestInfo *io, struct RastPort *rp,
	U32 widths[IO_CAPTURE_COLUMNS] )
{
	struct Node *row;
	struct USB2_NotifyMessage *entry;

	const char *request;
	const char *result;
	const char *type;
	const char *cells[IO_CAPTURE_COLUMNS];
	char sequence[12];
	char command[12];
	char device[10];
	char endpoint[20];
	char bytes[24];
	char error[12];
	U32 column;
	U32 width;

	result = "OK";

	request = Capture_Request_Name( io->nm_IOCommand, command, sizeof( command ) );

	if ( io->nm_State != USB2IO_Completed )
	{
		result = Active_IO_State( io->nm_State );

		snprintf( sequence, sizeof( sequence ), "%lu", (unsigned long) io->nm_HCD_Index );

		snprintf( bytes, sizeof( bytes ), "%lu", (unsigned long) io->nm_Length );
	}
	else
	{
		if ( io->nm_Error )
		{
			snprintf( error, sizeof( error ), "%d", (int) io->nm_Error );

			result = error;
		}

		snprintf( sequence, sizeof( sequence ), "%lu", (unsigned long) io->nm_Sequence );

		snprintf( bytes, sizeof( bytes ), "%lu/%lu", (unsigned long) io->nm_Actual, (unsigned long) io->nm_Length );
	}

	snprintf( device, sizeof( device ), "%04X:%04X", (unsigned) io->nm_VendorID, (unsigned) io->nm_ProductID );

	type = Capture_Endpoint_Type( io->nm_EndPointType );

	snprintf( endpoint, sizeof( endpoint ), "%02X %s", (unsigned) io->nm_EndPoint, type );

	cells[0] = sequence;
	cells[1] = request;
	cells[2] = device;
	cells[3] = endpoint;
	cells[4] = bytes;
	cells[5] = result;

	entry = AllocVecTags( sizeof( *entry ), TAG_END );

	if ( ! entry )
	{
		return ( NULL );
	}

	entry->nm_ID = id;
	entry->nm_Data.nm_IORequest = *io;

	row = AllocListBrowserNode( IO_CAPTURE_COLUMNS,
		LBNA_UserData, entry,
		LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, cells[0],
		LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, cells[1],
		LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, cells[2],
		LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, cells[3],
		LBNA_Column, 4, LBNCA_CopyText, TRUE, LBNCA_Text, cells[4],
		LBNA_Column, 5, LBNCA_CopyText, TRUE, LBNCA_Text, cells[5],
		TAG_END );

	if ( row )
	{
		for( column = 0; column < IO_CAPTURE_COLUMNS; column++ )
		{
			width = Capture_Text_Width( rp, cells[column] );
			widths[column] = MAX( widths[column], width );
		}

	}
	else
	{
		FreeVec( entry );
	}

	return ( row );
}

// --

U32
Append_IO_Capture( struct List *list, const struct USB2_NotifyMessage *msg, struct RastPort *rp,
	U32 widths[IO_CAPTURE_COLUMNS] )
{
	struct Node *row;

	row = Build_Capture_Row( msg->nm_ID, &msg->nm_Data.nm_IORequest, rp, widths );

	if ( row )
	{
		if ( CaptureCount == IO_CAPTURE_ROWS )
		{
			Free_Capture_Row( RemHead( list ) );
		}
		else
		{
			CaptureCount++;
		}

		AddTail( list, row );
	}

	return ( CaptureCount );
}

// --

static U32
Active_Sort_Key( const struct USB2_IORequestInfo *io )
{
	switch( ActiveSortColumn )
	{
		case 0: return ( io->nm_HCD_Index );
		case 1: return ( io->nm_IOCommand );
		case 2: return ( ( (U32) io->nm_VendorID << 16 ) | io->nm_ProductID );
		case 3: return ( ( (U32) io->nm_EndPoint << 8 ) | io->nm_EndPointType );
		case 4: return ( io->nm_Length );
		case 5: return ( io->nm_State );
	}

	return ( 0 );
}

// --

static int
Compare_Active_Requests( const void *left, const void *right )
{
	const struct USB2_ActiveIORequest *a;
	const struct USB2_ActiveIORequest *b;

	U32 first;
	U32 second;
	int order;

	a = left;
	b = right;

	first = Active_Sort_Key( &a->Info );

	second = Active_Sort_Key( &b->Info );

	order = ( first > second ) - ( first < second );

	return ( ActiveSortReverse ? -order : order );
}

// --

U32
Build_Active_IO_Requests( struct List *list, struct RastPort *rp, U32 widths[IO_CAPTURE_COLUMNS],
	U32 sortColumn, S32 sortReverse )
{
	struct USB2_ActiveIORequest snapshot[32];
	struct USB2_ActiveIORequest *entries;
	struct Node *row;

	U32 bytes;
	U32 capacity;
	U32 count;
	U32 index;

	entries = snapshot;
	capacity = sizeof( snapshot );

	bytes = USB2_Attr_Get( 0, USB2Tag_Attr_ActiveIORequests, entries, capacity );

	if ( bytes > capacity )
	{
		entries = AllocVecTags( bytes, TAG_END );

		if ( entries )
		{
			capacity = bytes;

			bytes = USB2_Attr_Get( 0, USB2Tag_Attr_ActiveIORequests, entries, capacity );
		}
		else
		{
			entries = snapshot;
		}
	}

	count = MIN( bytes, capacity ) / sizeof( *entries );

	ActiveSortColumn = sortColumn;
	ActiveSortReverse = sortReverse;

	qsort( entries, count, sizeof( *entries ), Compare_Active_Requests );

	for( index = 0; index < count; index++ )
	{
		row = Build_Capture_Row( entries[index].FunctionID, &entries[index].Info, rp, widths );

		if ( ! row )
		{
			break;
		}

		AddTail( list, row );
	}

	if ( entries != snapshot )
	{
		FreeVec( entries );
	}

	return ( bytes / sizeof( *entries ) );
}

// --

static const char *
Capture_Result_Name( S8 error )
{
	switch( error )
	{
		case 0: return ( "OK" );
		case IOERR_ABORTED: return ( "Aborted" );
		case IOERR_NOCMD: return ( "Unsupported command" );
		case USB2Err_Host_Stall: return ( "Endpoint stalled" );
		case USB2Err_Host_CRCError: return ( "CRC error" );
		case USB2Err_Host_HostError: return ( "Host controller error" );
		case USB2Err_Host_Timeout: return ( "Timed out" );
		case USB2Err_Host_Overflow: return ( "Overflow" );
		case USB2Err_Host_RuntPacket: return ( "Runt packet" );
		case USB2Err_Host_NoBandWidth: return ( "Insufficient bandwidth" );
		case USB2Err_Stack_EndPointNotFound: return ( "Endpoint unavailable" );
		case USB2Err_Stack_NoMemory: return ( "Out of memory" );
		case USB2Err_Device_Detached: return ( "Device detached" );
		case USB2Err_Device_OwnershipChange: return ( "Device ownership changed" );
		case USB2Err_Device_Nak: return ( "Device returned NAK" );
	}

	return ( "USB error" );
}

// --

static void
Add_IO_Request_Detail( struct List *list, const char *label, const char *format, ... )
{
	struct Node *row;

	va_list ap;
	char value[96];

	va_start( ap, format );

	vsnprintf( value, sizeof( value ), format, ap );

	va_end( ap );

	row = AllocListBrowserNode( 2, LBNA_Flags, LBFLG_READONLY,
		LBNA_Column, 0, LBNCA_Text, label,
		LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, value,
		TAG_END );

	if ( row )
	{
		AddTail( list, row );
	}
}

// --

void
Build_IO_Request_Details( struct List *list, const struct USB2_NotifyMessage *entry )
{
	const struct USB2_IORequestInfo *io;

	char command[12];
	const char *request;
	const char *direction;

	io = &entry->nm_Data.nm_IORequest;
	direction = "Control";

	if ( io->nm_EndPointType != EPATT_Type_Control )
	{
		direction = ( io->nm_EndPoint & EPADR_Dir_In ) ? "IN" : "OUT";
	}

	request = Capture_Request_Name( io->nm_IOCommand, command, sizeof( command ) );

	Add_IO_Request_Detail( list, "Command", "%s (%u)", request, (unsigned) io->nm_IOCommand );

	if ( io->nm_State == USB2IO_Completed )
	{
		Add_IO_Request_Detail( list, "Request #", "%lu", (unsigned long) io->nm_Sequence );

		Add_IO_Request_Detail( list, "Result", "%s (%d)", Capture_Result_Name( io->nm_Error ), (int) io->nm_Error );
	}
	else
	{
		Add_IO_Request_Detail( list, "State", "%s", Active_IO_State( io->nm_State ) );
	}

	Add_IO_Request_Detail( list, "Device", "%04X:%04X", (unsigned) io->nm_VendorID, (unsigned) io->nm_ProductID );

	Add_IO_Request_Detail( list, "USB function ID", "0x%016llX", (unsigned long long) entry->nm_ID );

	Add_IO_Request_Detail( list, "Host controller", "%lu", (unsigned long) io->nm_HCD_Index );

	Add_IO_Request_Detail( list, "USB address", "%u", (unsigned) io->nm_Address );

	if ( io->nm_Interface != 0xffff )
	{
		Add_IO_Request_Detail( list, "Interface", "%u", (unsigned) io->nm_Interface );
	}

	Add_IO_Request_Detail( list, "Endpoint", "0x%02X (%s)", (unsigned) io->nm_EndPoint, direction );

	Add_IO_Request_Detail( list, "Transfer type", "%s", Capture_Endpoint_Type( io->nm_EndPointType ) );

	Add_IO_Request_Detail( list, "Requested bytes", "%lu", (unsigned long) io->nm_Length );

	if ( io->nm_State == USB2IO_Completed )
	{
		Add_IO_Request_Detail( list, "Transferred bytes", "%lu", (unsigned long) io->nm_Actual );
	}
}

// --
