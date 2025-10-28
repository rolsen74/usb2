
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_USB2_MSGPORT_H
#define INC_PRIVATE_USB2_MSGPORT_H

/***************************************************************************/

struct USB2_MsgPort
{
	U32								ump_StructID;
	struct MsgPort					ump_MsgPort;
	struct USB2_Signal				ump_Signal;
};

/***************************************************************************/

#endif // INC_PRIVATE_USB2_MSGPORT_H
