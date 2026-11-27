#ifndef INTERFACES_BLUETOOTH2CONTROL_H
#define INTERFACES_BLUETOOTH2CONTROL_H

#include <interfaces/bluetooth2prefs.h>
#include <interfaces/bluetooth2sharecontrol.h>

#define BT2_SCAN_CAPACITY 64

enum BT2ManagedOperation
{
	BT2_MANAGED_SCAN,
	BT2_MANAGED_PAIR,
	BT2_MANAGED_WAV,
};

struct BT2ManagedAdapter
{
	struct BT2PrefsAdapter Adapter;
	uint32_t Registration;
};

struct BT2OperationStatus
{
	enum BTHCIResult Result;
	struct BTHCIError Error;
	uint32_t Count;
	uint8_t Pending;
	uint8_t Truncated;
};

struct BT2StartupShare
{
	uint8_t Enabled;
	uint8_t Local[6];
	char Path[BT_SHARE_PATH_SIZE];
};

struct BT2AudioOutput
{
	uint8_t Selected;
	uint8_t Local[6];
	uint8_t Peer[6];
};

enum BT2StartupShareState
{
	BT2_BOOT_SHARE_DISABLED,
	BT2_BOOT_SHARE_WAITING,
	BT2_BOOT_SHARE_CONFLICT,
	BT2_BOOT_SHARE_STOPPED,
	BT2_BOOT_SHARE_NEXT_BOOT,
	BT2_BOOT_SHARE_FOLDER_ERROR,
	BT2_BOOT_SHARE_RUNNING,
};

#endif
