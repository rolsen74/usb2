
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

#ifndef INC_PRIVATE_USB2_SIGNAL_H
#define INC_PRIVATE_USB2_SIGNAL_H

/***************************************************************************/

struct USB2_Signal
{
	U32								sig_StructID;
	S32								sig_Signal_Nr;
	U32								sig_Signal_Mask;
};

/***************************************************************************/

#endif // INC_PRIVATE_USB2_SIGNAL_H
