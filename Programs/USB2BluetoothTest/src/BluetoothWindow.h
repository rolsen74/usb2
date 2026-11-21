#ifndef USB2_BLUETOOTH_WINDOW_H
#define USB2_BLUETOOTH_WINDOW_H

#include <dos/dosextens.h>
#include <exec/semaphores.h>
#include "BluetoothTest.h"

#define BT_WINDOW_PEERS 32
#define BT_WINDOW_PATH 1024

enum BTWindowAction
{
	BT_WINDOW_SCAN,
	BT_WINDOW_SEND,
	BT_WINDOW_PLAY,
};

struct BTWindowJob
{
	struct DeathMessage Death;
	struct SignalSemaphore Lock;
	struct Task *Worker;
	struct BTInquiryDevice Peers[BT_WINDOW_PEERS];
	enum BTWindowAction Action;
	enum BTHCIResult DiscoveryResult;
	ULONG Unit;
	ULONG Channel;
	ULONG Count;
	LONG Cancelled;
	LONG Result;
	char Address[18];
	char Path[BT_WINDOW_PATH];
};

LONG StartBluetoothJob( struct BTWindowJob *job, struct MsgPort *reply );
void CancelBluetoothJob( struct BTWindowJob *job );

#endif
