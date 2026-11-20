
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "SAE.h"

// --

const APTR WPAInterfaceVectors[] =
{
	WPA_InterfaceObtain, WPA_InterfaceRelease, NULL, NULL,
	WPA3_Create, WPA3_Destroy, WPA3_AssociationIEs, WPA3_Process,
	WPA3_SAEStart, WPA3_SAECommit, WPA3_SAEConfirm, WPA3_SAEDestroy,
	WPA3_SAEStartMode, (APTR) -1
};

// --
