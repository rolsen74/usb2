
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static const U8 *
Library_Association_IE( struct USB2WPA2IFace *self __attribute__((unused)) )
{
	return ( AssociationRSN );
}

// --

const APTR WPAInterfaceVectors[] =
{
	WPA_InterfaceObtain, WPA_InterfaceRelease, NULL, NULL,
	Library_Association_IE, WPA2_Create, WPA2_Destroy, WPA2_Process, (APTR) -1
};

// --
