
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE void __HCD_Controllers_Stop( struct USBBase *usbbase )
{
struct USB2_HCDNode *next;
struct USB2_HCDNode *hn;
struct USB2_TaskNode *tn;
struct PCIResourceRange *range;
struct PCIDevice *pcidev;

U32 locked;

	USBINFO( "__HCD_Controllers_Stop   :  1 : Send CTRL to HCDs" );
	TASK_NAME_ENTER( "HCD : __HCD_Controllers_Stop" );

	hn = Header_Head( & usbbase->usb_HCDHeader );

	while( hn )
	{
		next = Node_Next( hn );

		SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

		tn = hn->hn_Task;

		if ( ( tn ) && ( TASK_LOCK( tn ) != LSTAT_Okay ) )
		{
			tn = NULL;
		}

		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		if ( tn )
		{
			TASK_STOP_TN( tn );

			TASK_UNLOCK( tn );
		}

		hn = next;
	}

	USBINFO( "__HCD_Controllers_Stop   :  2 : Wait for HCDs to stop" );

	ASYNC_WAIT( & usbbase->usb_HCDASync );

	USBINFO( "__HCD_Controllers_Stop   :  3 : HCDs have stopped, now free HN structs" );

	while( TRUE )
	{
		hn = NODE_REMHEAD( & usbbase->usb_HCDHeader );

		if ( ! hn )
		{
			break;
		}

		pcidev = hn->hn_PCIDevice;
		range = hn->hn_PCIDevResource;
		locked = hn->hn_PCIDevLock;

		if ( HCD_FREE( hn ) != FSTAT_Okay )
		{
			NODE_ADDTAIL( & usbbase->usb_HCDHeader, hn );

			MISC_WAIT( 1000 );

			continue;
		}

		if ( pcidev )
		{
			if ( range )
			{
				pcidev->FreeResourceRange( range );
			}

			if ( locked )
			{
				pcidev->Unlock();
			}

			usbbase->usb_IPCI->FreeDevice( pcidev );
		}
	}

	USBINFO( "__HCD_Controllers_Stop   :  5 : Exit" );

	TASK_NAME_LEAVE();
}

// --
