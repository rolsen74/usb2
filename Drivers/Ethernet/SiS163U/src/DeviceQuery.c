
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

/* Supported commands */
const UWORD EthClass_Commands[] =
{
	CMD_FLUSH,
	S2_DEVICEQUERY,
	S2_GETSTATIONADDRESS,
	S2_CONFIGINTERFACE,
	S2_GETSPECIALSTATS,
	S2_GETGLOBALSTATS,
	S2_ONEVENT,
	S2_OFFLINE,
	NSCMD_DEVICEQUERY,
	S2_GETEXTENDEDGLOBALSTATS,
	S2_SETOPTIONS,
	S2_READMGMT,
	S2_WRITEMGMT,
	S2_GETCRYPTTYPES,
	0
};
