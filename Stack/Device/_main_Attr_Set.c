
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static U32 _main_Attr_Set( struct USB2IFace *Self UNUSED, USB2_ID id UNUSED, U32 tag UNUSED, PTR buffer UNUSED, U32 buffersize UNUSED )
{
	return( 0 );
}

// --
