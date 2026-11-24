
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

const UWORD EthClass_Commands[] =
{
	NSCMD_DEVICEQUERY,
	S2_DEVICEQUERY,
	S2_GETSTATIONADDRESS,
	S2_GETSPECIALSTATS,
	S2_GETGLOBALSTATS,
	S2_GETEXTENDEDGLOBALSTATS,
	S2_ONEVENT,
	S2_GETCRYPTTYPES,
	0
};

// --
