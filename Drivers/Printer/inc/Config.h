
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef INC_CONFIG_H
#define INC_CONFIG_H

// --

#include "All.h"

// --

/*
** /S - Switch (BOOL)
** /N - NUmber (U32)
** /K - Keyword (Optional)
** /A - Required
*/

#define CONFIGTEMPLATE			"VENDORID/K/N,DEVICEID/K/N,BUFFERSIZE/K/N"

struct ConfigOptions
{
	U32 *	VendorID;			// Force custom VendorID
	U32 *	DeviceID;			// Force custom DeviceID
	U32 *	BufferSize;			// Buffer size in kb ( 0 = Off, 1 = 1024 bytes )
};

// --

#endif
