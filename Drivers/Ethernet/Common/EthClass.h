
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_ETHCLASS_H
#define USB2_ETHCLASS_H

#include <interfaces/usb2.h>
#include <interfaces/usb2eth.h>

// --

struct EthClassPort
{
	struct IORequest Request;
	struct USB2EthIFace *Interface;
	struct USB2EthUnit *Unit;
};

S32 Resources_Init( void );
void Resources_Free( void );
U32 EthClass_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
S32 EthClass_Register( struct EthClassPort *port, const struct USB2EthOps *ops, PTR adapter,
	ULONG *number, ULONG type, const struct USB2_DriverMessage *msg );
void EthClass_Unregister( struct EthClassPort *port );

#endif
