
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef PRT_DEVICE_H
#define PRT_DEVICE_H

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <proto/utility.h>

#include <devices/newstyle.h>
#include <devices/parallel.h>
#include <devices/timer.h>
#include <interfaces/usb2port.h>
#include <dos/dostags.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// --

#include "Types.h"
#include "Version.h"
// --
#include "Debug.h"
#include "Buffer.h"
#include "Config.h"
#include "Printer.h"

// --


#define NR_of_TXs	3
#define PRINTER_TX_TIMEOUT_US 15000000

struct PrinterUnit
{
	ULONG							unit_UnitNr;
	struct USB2PrinterIFace *		unit_PortInterface;
	struct USB2PortUnit *			unit_PortUnit;

	struct MsgPort *				unit_PRT_Begin_MsgPort;
	U32								unit_PRT_Begin_MsgPortBit;

	struct MsgPort *				unit_PRT_Abort_MsgPort;
	U32								unit_PRT_Abort_MsgPortBit;

	struct USB2_Register *			unit_USB_Register;
	struct USB2_Interface *			unit_USB_Interface;


	struct USB2_IORequest *			unit_Bulk_Tx_Active[NR_of_TXs];	// Active USB2_IORequest's
	struct USB2_EPResource *		unit_Bulk_Tx_Resource;
	struct USB2_EPResource *		unit_Bulk_Rx_Resource;
	U32								unit_Dot4_PacketSize;
	U32								unit_Dot4_Credits;
	U8								unit_Dot4_Socket;
	U8								unit_Dot4_Active;
	U8								unit_Dot4_Ready;
	U32								unit_Dot4_RxUsed;
	U8								unit_Dot4_RxBuffer[DOT4_MAX_PACKET];



	// struct USB2_IORequest *
	struct List *					unit_USB_IOReq_FreeList;		// Unused USB IOReq's

	// Active USB IOReq's
	S32								unit_USB_IOReq_ActiveCount;		//

	// struct IOStdReq *
	struct List *					unit_PAR_IOReq_WaitingList;		// Waiting too be send to usb printer
	S32								unit_PAR_IOReq_WaitingCount;	//

	// struct PrtBuffer * - we are filling now
	struct PrtBuffer *				unit_Buffer_Active;				//

	// struct PrtBuffer * - Unused buffers after we have send to usb
	struct List *					unit_Buffer_FreeList;			//
	S32								unit_Buffer_FreeCount;			//

	// struct PrtBuffer *
	U32								unit_Buffer_Size;				// User set, default 16kb
	struct List *					unit_Buffer_WaitingList;		// Waiting too be send to usb printer
	S32								unit_Buffer_WaitingCount;		//


	U8								unit_Detached;				// USB Device disconnected
	U8								unit_PrinterStatus;
	U8								unit_LastByteSend;
	U8								unit_JobHasData;
	S8 unit_JobError;
	U8 unit_NewJob;
	U8 unit_FormFeed;
	U8 unit_CloseTimerOpen;
	U8 unit_CloseTimerPending;
	U8 unit_CloseAborting;
	struct MsgPort *unit_CloseTimerPort;
	struct TimeRequest *unit_CloseTimer;
};

// --


// --


// --

extern struct Interface *INewlib;
extern struct Library *NewlibBase;


void	Cmd_Write_Buffer(				struct PrinterUnit *unit, struct IOStdReq *pario );
void	Cmd_Write_Direct(				struct PrinterUnit *unit, struct IOStdReq *pario );
S32		Resources_Init(					void );
void	Resources_Free(					void );
void	Buffer_Flush(					struct PrinterUnit *unit );
U32		Buffer_Write(					struct PrinterUnit *unit, U8 *data, U32 size );
void	Buffer_Schedule(				struct PrinterUnit *unit );
void	Direct_Schedule(				struct PrinterUnit *unit );
S32		Process_Setup_Device( struct PrinterUnit *unit, struct USB2_DriverMessage *msg );
S32		Dot4_Setup(					struct PrinterUnit *unit );
S32		Dot4_Write(					struct PrinterUnit *unit, const U8 *data, U32 size, U32 *written );
void	Dot4_Shutdown(					struct PrinterUnit *unit );
U32		Printer_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
void	Process_Main( struct PrinterUnit *unit );
void	Printer_Abort( struct PrinterUnit *unit );
S32 Send_FormFeed( struct PrinterUnit *unit );
void Printer_FormFeedDone( struct PrinterUnit *unit, struct USB2_IORequest *usbio );
S32 Printer_CloseTimerInit( struct PrinterUnit *unit );
void Printer_CloseTimerFree( struct PrinterUnit *unit );
void Printer_CloseTimerArm( struct PrinterUnit *unit, U32 seconds );
void Printer_CloseTimerStop( struct PrinterUnit *unit );

// --

#endif
