
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_USB_H
#define INC_USB_H

// --

enum
{
	TYPE_HCD,
	TYPE_FUNCTION,
	TYPE_INTERFACE,
	TYPE_LAST
};

// --

struct USB2_FktNode
{
	struct Node						Node;
	USB2_ID							ID;
	U32								Type;
	STR								Name;
	U32								Port;		// Port Nr 1-xx
	U32								Ports;		// Valid for HUB Class
	U32								Speed;
	U32								Config;
	struct List						Header;

	struct USB2_FktNode **			Functions;	 // HUB Only

	char							ManufacturerStr[127];
	char							ProductStr[127];
	char							SerialStr[127];

	struct USB2_Config_Desc *		CfgDesc;
	S32								CfgSize;

	struct USB2_Interface_Desc *	IfcDesc;
	S32								IfcSize;

	struct USB2_Device_Desc			DevDsc;
};

// --

#endif
