
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

#ifndef DRIVERS_OHCI_OHCI_H
#define DRIVERS_OHCI_OHCI_H

/***************************************************************************/

// //#ifdef DO_DEBUG
// #if 1

// void OHCI_Dump_Setup(	struct USB2_HCDNode *hn, struct USB2_SetupData *sd );
// void OHCI_Dump_QH(		struct USB2_HCDNode *hn, struct OHCI_QH *qh, int DoSetup );
// void OHCI_Dump_TD(		struct USB2_HCDNode *hn, struct OHCI_TD *td );

// #define OHCI_DUMP_SETUP(x,y)	OHCI_Dump_Setup(x,y)
// #define OHCI_DUMP_QH(x,y,z)		OHCI_Dump_QH(x,y,z)
// #define OHCI_DUMP_TD(x,y)		OHCI_Dump_TD(x,y)

// #else

// #define OHCI_DUMP_SETUP(x,y)	((void)0)
// #define OHCI_DUMP_QH(x,y,z)		((void)0)
// #define OHCI_DUMP_TD(x,y)		((void)0)

// #endif

// #if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

// SEC_CODE void	OHCI_Door_Bell_Init( struct USB2_HCDNode *hn, STR file );
// SEC_CODE void	OHCI_Door_Bell_Wait( struct USB2_HCDNode *hn, STR file );

// #else

// SEC_CODE void	OHCI_Door_Bell_Init( struct USB2_HCDNode *hn );
// SEC_CODE void	OHCI_Door_Bell_Wait( struct USB2_HCDNode *hn );

// #endif

// SEC_CODE void	OHCI_Bulk_Add( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE S32	OHCI_Bulk_Build( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE U32	OHCI_Bulk_Length( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Bulk_Remove( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Control_Add( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE S32	OHCI_Control_Build( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE U32	OHCI_Control_Length( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Control_Remove( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Interrupt_Add( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE S32	OHCI_Interrupt_Build( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE U32	OHCI_Interrupt_Length( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Interrupt_Remove( struct USB2_HCDNode *hn, struct RealRequest *ioreq );

SEC_CODE S32	OHCI_Chip_Preinit( struct USB2_HCDNode *hn );
SEC_CODE S32	OHCI_Chip_Reset( struct USB2_HCDNode *hn );
SEC_CODE S32	OHCI_Chip_Alloc( struct USB2_HCDNode *hn );
SEC_CODE S32	OHCI_Chip_Init( struct USB2_HCDNode *hn );
SEC_CODE S32	OHCI_Chip_Start( struct USB2_HCDNode *hn );
// SEC_CODE S32	OHCI_Chip_Stop( struct USB2_HCDNode *hn );
// SEC_CODE void	OHCI_Chip_Deinit( struct USB2_HCDNode *hn );
// SEC_CODE void	OHCI_Chip_Dealloc( struct USB2_HCDNode *hn );

// SEC_CODE void	OHCI_Port_Disown( struct USB2_HCDNode *hn, U32 port );

SEC_CODE S32	OHCI_Port_Clr_Enable( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Suspend( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Power( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Indicator( struct USB2_HCDNode *hn, U32 port );

SEC_CODE S32	OHCI_Port_Clr_Enable_Chg( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Suspend_Chg( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_OverCurrent_Chg( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Connect_Chg( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Clr_Reset_Chg( struct USB2_HCDNode *hn, U32 port );

SEC_CODE U32	OHCI_Port_Get_Status( struct USB2_HCDNode *hn, U32 port );

SEC_CODE S32	OHCI_Port_Set_Enable( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Set_Suspend( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Set_Reset( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Set_Power( struct USB2_HCDNode *hn, U32 port );
SEC_CODE S32	OHCI_Port_Set_Indicator( struct USB2_HCDNode *hn, U32 port );

// SEC_CODE S32	OHCI_Get_20kBuffer( struct USB2_HCDNode *hn, struct OHCI_ITD *itd, struct OHCI_TD *td, PTR data, U32 len );
SEC_CODE struct OHCI_ED *OHCI_Get_EDBuffer( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
SEC_CODE struct OHCI_TD *OHCI_Get_TDBuffer( struct USB2_HCDNode *hn );

// SEC_CODE S32	OHCI_Enough_Bandwidth( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE S32	OHCI_FindSlot( struct USB2_HCDNode *hn, U32 interval );
// SEC_CODE void	OHCI_FreeSlot( struct USB2_HCDNode *hn, U32 slot );
// SEC_CODE void	OHCI_Handler_HCD( struct USB2_HCDNode *hn, U32 mask );
SEC_CODE U32	OHCI_Handler_Interrupt( struct ExceptionContext *Context, struct ExecBase *SysBase, PTR userData );
SEC_CODE U32	OHCI_Handler_Reset( struct ExceptionContext *Context, struct ExecBase *SysBase, PTR userData );
// SEC_CODE void	OHCI_Periodic_Start( struct USB2_HCDNode *hn );
// SEC_CODE void	OHCI_Periodic_Stop( struct USB2_HCDNode *hn );
// SEC_CODE S32	OHCI_Transfer_Check( struct USB2_HCDNode *hn, struct RealRequest *ioreq );
// SEC_CODE void	OHCI_Transfer_Free( struct USB2_HCDNode *hn, struct RealRequest *ioreq );

/***************************************************************************/

#endif // DRIVERS_OHCI_OHCI_H
