
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// The caller holds the stack lock.
SEC_CODE const struct USB2_InterfaceHeader *
USB2_IORequest_Interface( const struct RealRequest *ioreq )
{
	const struct USB2_EndPointNode *ep;

	ep = ioreq->req_EndPoint;

	if ( ep->ep_Interface )
	{
		return ( ep->ep_Interface->in_Parent );
	}

	if ( ( ep->ep_Type == EPATT_Type_Control ) && ( ioreq->req_Resource ) )
	{
		// Endpoint zero is shared. Use the request's interface registration.
		return ( ioreq->req_Resource->epr_Register->reg_Interface );
	}

	return ( NULL );
}

// --

SEC_CODE void
USB2_IORequest_Describe( const struct RealRequest *ioreq, const struct USB2_InterfaceHeader *interface,
	struct USB2_IORequestInfo *info )
{
	const struct RealFunctionNode *fn;
	const struct USB2_EndPointNode *ep;
	const struct USB2_Device_Desc *desc;

	fn = ioreq->req_Function;
	ep = ioreq->req_EndPoint;
	desc = fn->fkt_DeviceDescriptor;

	*info = (struct USB2_IORequestInfo) { 0 };
	info->nm_HCD_Index = fn->fkt_HCD ? fn->fkt_HCD->hn_HCDIndex : ~0U;
	info->nm_Length = ioreq->req_Public.io_Length;
	info->nm_VendorID = desc ? LE_SWAP16( desc->VendorID ) : 0;
	info->nm_ProductID = desc ? LE_SWAP16( desc->DeviceID ) : 0;
	info->nm_IOCommand = ioreq->req_Public.io_Command;
	info->nm_Address = fn->fkt_Address;
	info->nm_EndPoint = ep->ep_Number | ( ep->ep_Direction == EPDIR_In ? EPADR_Dir_In : EPADR_Dir_Out );
	info->nm_EndPointType = ep->ep_Type;
	info->nm_Interface = interface ? interface->ih_Number : 0xffff;
}

// --
