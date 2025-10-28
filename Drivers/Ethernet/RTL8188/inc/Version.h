
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef INC_VERSION_H
#define INC_VERSION_H

// --

#if defined(_USB2_)
#define DEVNAME		"usb2_rtl8188.device"
#elif defined(_SIRION_)
#define DEVNAME		"rusb_rtl8188.device"
#else
#error Unsupported USB Target
#endif

#define VERSION		1
#define REVISION	1
#define DATE		"27.8.2025"
#define VERS		DEVNAME " 1.1"
#define VSTRING		DEVNAME " 1.1 (" DATE ")\r\n"
#define VERSTAG		"\0$VER: " DEVNAME " 1.1 (" DATE ")"

// --

#endif
