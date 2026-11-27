#ifndef INTERFACES_BLUETOOTH2SHARECONTROL_H
#define INTERFACES_BLUETOOTH2SHARECONTROL_H

#include <interfaces/bluetooth2share.h>

enum BTShareState
{
	BT_SHARE_STOPPED,
	BT_SHARE_STARTING,
	BT_SHARE_LISTENING,
	BT_SHARE_STOPPING,
};

struct BTShareStatus
{
	enum BTShareState State;
	enum BTHCIResult Result;
	struct BTHCIError Error;
	uint8_t Local[6];
	char Path[BT_SHARE_PATH_SIZE];
};

#endif
