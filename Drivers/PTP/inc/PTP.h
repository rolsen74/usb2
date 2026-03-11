
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_PTP_H
#define INC_PTP_H

// --

enum ContainerType
{
	CTYPE_Command = 1,
	CTYPE_Data,
	CTYPE_Response,
	CTYPE_Event,
};

#define PTP_Get_Device_Info		0x1001
#define PTP_Open_Session		0x1002
#define PTP_Close_Session		0x1003
#define PTP_Response_OK			0x2001

#pragma pack(1)

struct PTP_Container
{
	U32		Length;
	U16		Type;
	U16		Code;
	U32		TransID;
	U32		Parms[5];
};

#pragma pack(0)

// --

#endif
