
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#include "__Int_Read.c"

// --

SEC_CODE void __RootHUB_Handle_Chg( struct USBBase *usbbase, struct USB2_HCDNode *hn )
{
struct RealRequest *ioreq;
struct RealRequest *next;
S32 len;

	USBDEBUG( "__RootHUB_Handle_Chg : Enter" );

	ioreq = hn->hn_HUB_Interrupts.uh_Head;

	while( ioreq )
	{
		next = Node_Next( ioreq );
		// --

		len = __Interrupt_Read( usbbase, hn, ioreq->req_Public.io_Data, ioreq->req_Public.io_Length );

		if ( len > 0 )
		{
			NODE_REMNODE( & hn->hn_HUB_Interrupts, ioreq );

			ioreq->req_Public.io_Actual = len;
			ioreq->req_Public.io_Error = USB2Err_NoError;
//			ioreq->req_PublicStat = IORS_User;
//			MSGPORT_REPLYMSG(ioreq);
//			IOREQUEST_ACTIVE_SUB( ioreq );

			HCD_REPLY( ioreq );
			ioreq = NULL;
		}

		// --
		ioreq = next;
	}

	USBDEBUG( "__RootHUB_Handle_Chg : Leave" );
}

// --

