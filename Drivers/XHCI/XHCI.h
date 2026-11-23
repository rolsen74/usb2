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

#ifndef DRIVERS_XHCI_XHCI_H
#define DRIVERS_XHCI_XHCI_H

/***************************************************************************/

#define XHCI_CAPLENGTH		  0x00
#define XHCI_HCSPARAMS1		  0x04
#define XHCI_HCSPARAMS2		  0x08
#define XHCI_HCCPARAMS1		  0x10
#define XHCI_DBOFF			  0x14
#define XHCI_RTSOFF			  0x18

#define XHCI_USBCMD			  0x00
#define XHCI_USBSTS			  0x04
#define XHCI_PAGESIZE		  0x08
#define XHCI_CRCR			  0x18
#define XHCI_DCBAAP			  0x30
#define XHCI_CONFIG			  0x38
#define XHCI_PORTSC( port )	  ( 0x400 + ( ( ( port ) - 1 ) * 0x10 ) )

#define XHCI_IMAN			  0x20
#define XHCI_IMOD			  0x24
#define XHCI_ERSTSZ			  0x28
#define XHCI_ERSTBA			  0x30
#define XHCI_ERDP			  0x38

#define XHCI_CMD_RUN		  0x00000001UL
#define XHCI_CMD_RESET		  0x00000002UL
#define XHCI_CMD_INT_ENABLE	  0x00000004UL
#define XHCI_STS_HALTED		  0x00000001UL
#define XHCI_STS_HOST_ERROR	  0x00000004UL
#define XHCI_STS_EVENT		  0x00000008UL
#define XHCI_STS_PORT_CHANGE  0x00000010UL
#define XHCI_STS_NOT_READY	  0x00000800UL
#define XHCI_IMAN_PENDING	  0x00000001UL
#define XHCI_IMAN_ENABLE	  0x00000002UL
#define XHCI_ERDP_BUSY		  0x00000008UL

#define XHCI_PORT_CONNECTED	  0x00000001UL
#define XHCI_PORT_ENABLED	  0x00000002UL
#define XHCI_PORT_OVERCURRENT 0x00000008UL
#define XHCI_PORT_RESET		  0x00000010UL
#define XHCI_PORT_POWER		  0x00000200UL
#define XHCI_PORT_SPEED( x )  ( ( ( x ) >> 10 ) & 15 )
#define XHCI_PORT_CSC		  0x00020000UL
#define XHCI_PORT_PEC		  0x00040000UL
#define XHCI_PORT_OCC		  0x00100000UL
#define XHCI_PORT_PRC		  0x00200000UL
#define XHCI_PORT_WRC		  0x00080000UL
#define XHCI_PORT_PLC		  0x00400000UL
#define XHCI_PORT_CEC		  0x00800000UL
#define XHCI_PORT_CHANGE_BITS                                                                                                   \
	( XHCI_PORT_CSC | XHCI_PORT_PEC | XHCI_PORT_WRC | XHCI_PORT_OCC | XHCI_PORT_PRC | XHCI_PORT_PLC | XHCI_PORT_CEC )
#define XHCI_PORT_WRITE_SAFE( x ) ( ( x ) & ~( XHCI_PORT_CHANGE_BITS | XHCI_PORT_ENABLED | XHCI_PORT_RESET | 0x80010000UL ) )

#define XHCI_TRB_CYCLE			  0x00000001UL
#define XHCI_TRB_TOGGLE			  0x00000002UL
#define XHCI_TRB_ISP			  0x00000004UL
#define XHCI_TRB_CHAIN			  0x00000010UL
#define XHCI_TRB_IOC			  0x00000020UL
#define XHCI_TRB_IDT			  0x00000040UL
#define XHCI_TRB_DIR_IN			  0x00010000UL
#define XHCI_TRB_TYPE( x )		  ( ( x ) << 10 )
#define XHCI_TRB_GET_TYPE( x )	  ( ( ( x ) >> 10 ) & 0x3f )
#define XHCI_TRB_NORMAL			  1
#define XHCI_TRB_SETUP			  2
#define XHCI_TRB_DATA			  3
#define XHCI_TRB_STATUS			  4
#define XHCI_TRB_ISOCH			  5
#define XHCI_TRB_LINK			  6
#define XHCI_TRB_ENABLE_SLOT	  9
#define XHCI_TRB_DISABLE_SLOT	  10
#define XHCI_TRB_ADDRESS_DEVICE	  11
#define XHCI_TRB_CONFIGURE_EP	  12
#define XHCI_TRB_EVALUATE_CTX	  13
#define XHCI_TRB_RESET_EP		  14
#define XHCI_TRB_STOP_EP		  15
#define XHCI_TRB_SET_DEQUEUE	  16
#define XHCI_TRB_TRANSFER_EVENT	  32
#define XHCI_TRB_COMMAND_EVENT	  33
#define XHCI_TRB_PORT_EVENT		  34

#define XHCI_CC_SUCCESS			  1
#define XHCI_CC_SHORT_PACKET	  13
#define XHCI_CC_STOPPED			  26
#define XHCI_TRB_SIA			  0x80000000UL

static inline void
XHCI_Sync ( void )
{
	__asm__ volatile ( "sync" ::: "memory" );
}

SEC_CODE S32  XHCI_Chip_Preinit ( struct USB2_HCDNode * hn );
SEC_CODE S32  XHCI_Chip_Reset ( struct USB2_HCDNode * hn );
SEC_CODE S32  XHCI_Chip_Alloc ( struct USB2_HCDNode * hn );
SEC_CODE S32  XHCI_Chip_Init ( struct USB2_HCDNode * hn );
SEC_CODE S32  XHCI_Chip_Start ( struct USB2_HCDNode * hn );
SEC_CODE S32  XHCI_Chip_Stop ( struct USB2_HCDNode * hn );
SEC_CODE void XHCI_Chip_Deinit ( struct USB2_HCDNode * hn );
SEC_CODE void XHCI_Chip_Dealloc ( struct USB2_HCDNode * hn );

SEC_CODE void XHCI_Handler_HCD ( struct USB2_HCDNode * hn, U32 mask );
SEC_CODE U32  XHCI_Handler_Interrupt ( struct ExceptionContext * context, struct ExecBase * sysbase, PTR data );
SEC_CODE U32  XHCI_Handler_Reset ( struct ExceptionContext * context, struct ExecBase * sysbase, PTR data );
SEC_CODE S32  XHCI_Transfer_Check ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Transfer_Free ( struct USB2_HCDNode * hn, struct RealRequest * req );

SEC_CODE S32				XHCI_Ring_Alloc ( struct USB2_HCDNode * hn, struct XHCI_Ring * ring );
SEC_CODE void				XHCI_Ring_Free ( struct USB2_HCDNode * hn, struct XHCI_Ring * ring );
SEC_CODE U32				XHCI_Ring_Queue ( struct XHCI_Ring * ring, U32 low, U32 high, U32 status, U32 control );
SEC_CODE void				XHCI_Event_Poll ( struct USB2_HCDNode * hn );
SEC_CODE S32				XHCI_Command ( struct USB2_HCDNode * hn, U32 low, U32 high, U32 status, U32 control, U32 * result );
SEC_CODE void				XHCI_Slot_Free ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot );
SEC_CODE S32				XHCI_Slot_Disable ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot );
SEC_CODE S32				XHCI_Slots_Retire ( struct USB2_HCDNode * hn );
SEC_CODE struct XHCI_Slot * XHCI_Slot_Find ( struct USB2_HCDNode * hn, USB2_ID function_id );
SEC_CODE struct XHCI_Slot * XHCI_Slot_Create ( struct USB2_HCDNode * hn, struct RealFunctionNode * fn );
SEC_CODE S32				XHCI_Endpoint_Drop ( struct USB2_HCDNode * hn, struct USB2_InterfaceHeader * ih );
SEC_CODE S32  XHCI_Endpoint_Ensure ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot, struct USB2_EndPointNode * ep,
									 U32 direction );
SEC_CODE S32  XHCI_Endpoint_Update0 ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot, U32 packet );
SEC_CODE S32  XHCI_Hub_Ensure ( struct USB2_HCDNode * hn, struct RealFunctionNode * fn );
SEC_CODE void XHCI_Endpoint_Stop ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE S32  XHCI_Buffer_Add ( struct USB2_HCDNode * hn, struct RealRequest * req, U32 offset, U32 length );

SEC_CODE U32 XHCI_Port_Get_Status ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Set_Reset ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Set_Power ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_Power ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_Enable ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_Connect_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_Enable_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_OverCurrent_Chg ( struct USB2_HCDNode * hn, U32 port );
SEC_CODE S32 XHCI_Port_Clr_Reset_Chg ( struct USB2_HCDNode * hn, U32 port );

SEC_CODE S32  XHCI_Control_Build ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Control_Add ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Control_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE U32  XHCI_Control_Length ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE S32  XHCI_Bulk_Build ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Bulk_Add ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Bulk_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE U32  XHCI_Bulk_Length ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE S32  XHCI_Interrupt_Build ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Interrupt_Add ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Interrupt_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE U32  XHCI_Interrupt_Length ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE S32  XHCI_Isochronous_Build ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Isochronous_Add ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE void XHCI_Isochronous_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req );
SEC_CODE U32  XHCI_Isochronous_Length ( struct USB2_HCDNode * hn, struct RealRequest * req );

#endif // DRIVERS_XHCI_XHCI_H
