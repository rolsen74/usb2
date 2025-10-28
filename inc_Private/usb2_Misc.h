
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

#ifndef INC_PRIVATE_USB2_MISC_H
#define INC_PRIVATE_USB2_MISC_H

/***************************************************************************/

struct RealSetupData
{
	struct USB2_SetupData			rsd_Public;					// At the moment must be first, todo: do same as EPResource or ?
	U32								rsd_StructID;
	U32								rsd_Phy;
};

/***************************************************************************/

#endif // INC_PRIVATE_USB2_MISC_H
