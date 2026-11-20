
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "WPA.h"

// --

static const U8 *
Library_Association_IE( struct USB2WPAIFace *self __attribute__((unused)) )
{
	return ( AssociationWPA );
}

// --

const APTR WPAInterfaceVectors[] =
{
	WPA_InterfaceObtain, WPA_InterfaceRelease, NULL, NULL,
	Library_Association_IE, WPA_Create, WPA_Destroy, WPA_Process, (APTR) -1
};

// --
