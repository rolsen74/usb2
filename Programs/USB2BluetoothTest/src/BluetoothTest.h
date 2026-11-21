#ifndef USB2_BLUETOOTH_TEST_H
#define USB2_BLUETOOTH_TEST_H

#include <dos/dos.h>
#include <interfaces/bluetooth2.h>

enum BTPairOperation
{
	BT_TEST_PAIR,
	BT_TEST_BOND,
	BT_TEST_RECONNECT,
};

LONG ParseAddress( const char *text, uint8_t *address );
LONG ReadRemoteName( struct IORequest *request, ULONG unit, const char *address );
LONG MakeVisible( struct IORequest *request, ULONG unit, const char *name, LONG seconds );
LONG L2CAPEcho( struct IORequest *request, ULONG unit, const char *address );
LONG BrowseServices( struct IORequest *request, ULONG unit, const char *address );
LONG TestRFCOMM( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *text, LONG reply );
LONG SendRFCOMMFile( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *path );
LONG CaptureRFCOMMFile( struct IORequest *request, ULONG unit, const char *address, LONG channel,
	const char *path, LONG bytes );
LONG PushObjectFile( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *path );
LONG ResolveRFCOMMSource( BPTR lock, char *path, ULONG length );
void ReportRFCOMMStage( uint8_t stage );
void ReportHCIError( enum BTHCIResult result, const struct BTHCIError *error );
LONG PairController( struct IORequest *request, ULONG unit, const char *address, enum BTPairOperation operation );

LONG RunLE( struct IORequest *request, ULONG unit, enum BTLEMode mode, const char *name, LONG seconds );

LONG RunBluetoothWindow( ULONG unit );

LONG PlayBluetoothAudio( struct IORequest *request, ULONG unit, const char *address, const char *path );

LONG MonitorBluetoothHID( struct IORequest *request, ULONG unit, const char *address, LONG seconds );

LONG ConnectBluetoothInput( struct IORequest *request, ULONG unit, const char *address );

#endif
