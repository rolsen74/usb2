
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
SEC_CODE static S32
InterfaceBusy( struct USBBase *usbbase, struct RealRegister *reg, struct USB2_InterfaceHeader *ih )
{
	struct RealEndPointResource *epr;
	struct USB2_EndPointNode *ep;
	struct RealRequest *ioreq;
	struct USB2_Node *node;

	S32 last_claim;

	if ( ih->ih_Changing )
	{
		return ( TRUE );
	}

	for( epr = reg->reg_EPRHeader.uh_Head; epr; epr = Node_Next( epr ) )
	{
		ep = epr->epr_EndPoint;

		if ( ( ep->ep_Interface ) && ( ep->ep_Interface->in_Parent == ih ) )
		{
			return ( TRUE );
		}
	}

	last_claim = ! Register_HasControlClaim( reg, ih );

	// A resource can be unlinked while its requests are still draining.
	for( node = usbbase->usb_IORequests.uh_Head; node; node = Node_Next( node ) )
	{
		ioreq = node->un_Data;
		ep = ioreq->req_EndPoint;

		if ( ( ioreq->req_PublicStat != IORS_User ) && ( ep->ep_Interface ) && ( ep->ep_Interface->in_Parent == ih ) )
		{
			return ( TRUE );
		}

		if ( ( last_claim ) && ( ioreq->req_PublicStat != IORS_User ) &&
			( ep == ih->ih_Function->fkt_ControlEndPoint ) && ( ioreq->req_Resource ) &&
			( ioreq->req_Resource->epr_Register == reg ) )
		{
			return ( TRUE );
		}

		ep = ioreq->req_DestallTarget;

		if ( ( ep ) && ( ep->ep_Interface ) && ( ep->ep_Interface->in_Parent == ih ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE S32
__Interface_DeclaimHeader( struct USBBase *usbbase, struct RealRegister *reg, struct USB2_InterfaceHeader *ih )
{
	struct RealFunctionNode *fn;

	S32 retval;
	S32 closing;

	retval = FALSE;

	if ( REGISTER_LOCK( reg ) != LSTAT_Okay )
	{
		return ( FALSE );
	}

	if ( INTERFACE_LOCKHEADER( ih ) == LSTAT_Okay )
	{
		USBDEBUG( "__Interface_DeclaimHeader : Registration %p, interface %p", reg, ih );

		fn = ih->ih_Function;

		SEMAPHORE_OBTAIN( & fn->fkt_Semaphore );

		SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

		// Unregister has already drained this register and waited for its users.
		closing = ( reg->reg_FreeMe ) && ( reg->reg_ClosingTask == TASK_FIND() );

		if ( ( ih->ih_Owner == reg ) && ( ( closing ) || ( ( ! reg->reg_FreeMe ) && ( ! InterfaceBusy( usbbase, reg, ih ) ) ) ) )
		{
			ih->ih_Owner = NULL;
			fn->fkt_InterfaceClaimLocks--;
			retval = TRUE;

			INTERFACE_UNLOCKHEADER( ih );
		}

		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		SEMAPHORE_RELEASE( & fn->fkt_Semaphore );

		INTERFACE_UNLOCKHEADER( ih );
	}

	REGISTER_UNLOCK( reg );

	return ( retval );
}

// --
