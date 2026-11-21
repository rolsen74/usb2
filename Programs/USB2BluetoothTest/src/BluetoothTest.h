#ifndef USB2_BLUETOOTH_TEST_H
#define USB2_BLUETOOTH_TEST_H

#include <interfaces/bluetooth2.h>

enum BTPairOperation
{
	BT_TEST_PAIR,
	BT_TEST_BOND,
	BT_TEST_RECONNECT,
};

void ReportHCIError( enum BTHCIResult result, const struct BTHCIError *error );
LONG PairController( struct IORequest *request, ULONG unit, const char *address, enum BTPairOperation operation );

#endif
