
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
#define PTP_Get_Storage_IDs		0x1004
#define PTP_Get_Storage_Info		0x1005
#define PTP_Get_Object_Handles	0x1007
#define PTP_Get_Object_Info		0x1008
#define PTP_Get_Object			0x1009
#define PTP_Send_Object_Info		0x100c
#define PTP_Send_Object			0x100d
#define PTP_Get_Partial_Object	0x101b

#define PTP_Response_Unsupported	0x2005
#define PTP_Response_InvalidHandle	0x2009
#define PTP_Response_Incomplete	0x2007
#define PTP_Response_InvalidStorage	0x2008
#define PTP_Response_InvalidFormat	0x200b
#define PTP_Response_StoreFull		0x200c
#define PTP_Response_WriteProtected	0x200d
#define PTP_Response_StoreReadOnly	0x200e
#define PTP_Response_AccessDenied	0x200f
#define PTP_Response_StoreUnavailable	0x2013
#define PTP_Response_Busy			0x2019
#define PTP_Response_InvalidParent	0x201a
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
