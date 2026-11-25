
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INTERFACES_USB2MEDIA_H
#define INTERFACES_USB2MEDIA_H

#include <exec/interfaces.h>
#include <exec/io.h>

// Open this unit with an IOStdReq for NSCMD_DEVICEQUERY without hardware.
#define USB2_MEDIA_QUERY_UNIT 0xffffffffUL

// The registered backend and its manager must remain resident.
struct USB2MediaIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2MediaIFace *Self );
	ULONG APICALL (*Release)( struct USB2MediaIFace *Self );
	void APICALL (*Expunge)( struct USB2MediaIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2MediaIFace *Self );
	BOOL APICALL (*RegisterBackend)( struct USB2MediaIFace *Self, struct Device *backend );
};

#endif
