
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef DRIVERS_UHCI_UHCI_H
#define DRIVERS_UHCI_UHCI_H

/***************************************************************************/

#define USB_BUS_RESET_DELAY	  100
#define UHCI_RESET_TIMEOUT	  100
#define UHCI_VFRAMELIST_COUNT 128
#define UHCI_FRAMELIST_COUNT  1024
#define UHCI_FRAMELIST_MASK   ( UHCI_FRAMELIST_COUNT - 1 )
#define UHCI_PERIODIC_ANCHORS ( 2 * UHCI_VFRAMELIST_COUNT - 1 )
#define UHCI_PERIODIC_BITS    10800

// --

// //#ifdef DO_DEBUG
// #if 1

// void UHCI_Dump_Setup(	struct USB2_HCDNode *hn, struct USB2_SetupData *sd );
// void UHCI_Dump_QH(		struct USB2_HCDNode *hn, struct UHCI_QH *qh, int DoSetup );
// void UHCI_Dump_TD(		struct USB2_HCDNode *hn, struct UHCI_TD *td, S32 depth );

// #define UHCI_DUMP_SETUP(x,y)	UHCI_Dump_Setup(x,y)
// #define UHCI_DUMP_QH(x,y,z)		UHCI_Dump_QH(x,y,z)
// #define UHCI_DUMP_TD(x,y)		UHCI_Dump_TD(x,y)

// #else

// #define UHCI_DUMP_SETUP(x,y)	((void)0)
// #define UHCI_DUMP_QH(x,y,z)		((void)0)
// #define UHCI_DUMP_TD(x,y)		((void)0)

// #endif

// #if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

// SEC_CODE void	UHCI_Door_Bell_Init( struct USB2_HCDNode *hn, STR file );
// SEC_CODE void	UHCI_Door_Bell_Wait( struct USB2_HCDNode *hn, STR file );

// #else

// SEC_CODE void	UHCI_Door_Bell_Init( struct USB2_HCDNode *hn );
// SEC_CODE void	UHCI_Door_Bell_Wait( struct USB2_HCDNode *hn );

// #endif

SEC_CODE void UHCI_Bulk_Add ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Bulk_Build ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Bulk_Remove ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Control_Add ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Control_Build ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Control_Build_Low ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Control_Build_Full ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Control_Remove ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Interrupt_Add ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Interrupt_Build ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Interrupt_Remove ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32 UHCI_Isochronous_Build ( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
SEC_CODE void UHCI_Isochronous_Add ( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
SEC_CODE void UHCI_Isochronous_Remove ( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
SEC_CODE U32 UHCI_Isochronous_Length ( struct USB2_HCDNode *hn, struct RealRequest *ioreq );

SEC_CODE S32 UHCI_Chip_Preinit( struct USB2_HCDNode *hn );
SEC_CODE S32 UHCI_Chip_Reset ( struct USB2_HCDNode * hn );
SEC_CODE S32 UHCI_Chip_Alloc ( struct USB2_HCDNode * hn );
SEC_CODE S32 UHCI_Chip_Init ( struct USB2_HCDNode * hn );
SEC_CODE S32 UHCI_Chip_Start( struct USB2_HCDNode *hn );
SEC_CODE S32 UHCI_Chip_Stop( struct USB2_HCDNode *hn );
SEC_CODE void UHCI_Chip_Deinit( struct USB2_HCDNode *hn );
SEC_CODE void UHCI_Chip_Dealloc ( struct USB2_HCDNode * hn );

SEC_CODE S32 UHCI_Port_Clr_Enable ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Suspend ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Power ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Indicator ( struct USB2_HCDNode * hn, U32 port );

SEC_CODE S32 UHCI_Port_Clr_Enable_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Suspend_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_OverCurrent_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Connect_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Clr_Reset_Chg ( struct USB2_HCDNode * hn, U32 port );

SEC_CODE U32 UHCI_Port_Get_Status ( struct USB2_HCDNode * hn, U32 port );

SEC_CODE S32 UHCI_Port_Set_Enable ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Set_Suspend ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Set_Reset ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Set_Power ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 UHCI_Port_Set_Indicator ( struct USB2_HCDNode * hn, U32 port );

SEC_CODE S32 UHCI_Get_4kBuffer ( struct USB2_HCDNode * hn, struct UHCI_TD * td, PTR data, U32 len );
// SEC_CODE S32 UHCI_Get_20kBuffer ( struct USB2_HCDNode * hn, struct UHCI_ITD * itd, struct UHCI_TD * td, PTR data, U32 len );
SEC_CODE struct UHCI_QH * UHCI_Get_QHBuffer ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE struct UHCI_TD * UHCI_Get_TDBuffer ( struct USB2_HCDNode * hn );
SEC_CODE struct UHCI_TD *UHCI_TD_Init( struct USB2_HCDNode *hn, U32 pid, U32 length, U32 toggle, U32 speed, U32 address, U32 endpoint, U32 ioc, U32 short_ok );
SEC_CODE void UHCI_TD_Append( struct UHCI_QH *qh, struct UHCI_TD **tail, struct UHCI_TD *td, S32 depth );

SEC_CODE S32 UHCI_Periodic_Reserve( struct USB2_HCDNode *hn, struct RealRequest *req );
SEC_CODE void UHCI_Periodic_Release( struct USB2_HCDNode *hn, struct UHCI_QH *qh );
SEC_CODE void UHCI_Free_QH( struct USB2_HCDNode *hn, struct UHCI_QH *qh );
SEC_CODE void UHCI_Isochronous_Unlink_TD( struct USB2_HCDNode *hn, struct UHCI_TD *td );
SEC_CODE void UHCI_Link_QH( struct UHCI_QH *anchor, struct UHCI_QH *qh );
SEC_CODE S32 UHCI_Unlink_QH( struct UHCI_QH *qh );
SEC_CODE U32 UHCI_Data_Length( struct USB2_HCDNode *hn, struct RealRequest *req );
SEC_CODE S32  UHCI_Enough_Bandwidth ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE S32  UHCI_Slot_Find ( struct USB2_HCDNode * hn, U32 interval );
SEC_CODE void UHCI_Slot_Free ( struct USB2_HCDNode * hn, U32 slot );
SEC_CODE void UHCI_Handler_HCD ( struct USB2_HCDNode * hn, U32 mask );
SEC_CODE U32  UHCI_Handler_Interrupt ( struct ExceptionContext * Context, struct ExecBase * SysBase, PTR userData );
SEC_CODE U32  UHCI_Handler_Reset ( struct ExceptionContext * Context, struct ExecBase * SysBase, PTR userData );
SEC_CODE void UHCI_Periodic_Start ( struct USB2_HCDNode * hn );
SEC_CODE void UHCI_Periodic_Stop ( struct USB2_HCDNode * hn );
SEC_CODE S32  UHCI_Transfer_Check ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );
SEC_CODE void UHCI_Transfer_Free ( struct USB2_HCDNode * hn, struct RealRequest * ioreq );

/***************************************************************************/

#endif // DRIVERS_UHCI_UHCI_H
