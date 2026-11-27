#ifndef INTERFACES_BLUETOOTH2SHARE_H
#define INTERFACES_BLUETOOTH2SHARE_H

#include "Bluetooth.h"

#define BT_SHARE_PATH_SIZE 512
#define BT_SHARE_READ 1
#define BT_SHARE_WRITE 2

#define BT_SHARE_RULE_GET (-1)

struct BTFileShare
{
	uint32_t Cancel;
	uint32_t Listening;
	uint8_t Local[6];
	char Path[BT_SHARE_PATH_SIZE];
};

#endif
