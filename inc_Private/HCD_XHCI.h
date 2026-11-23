
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_HCD_XHCI_H
#define INC_PRIVATE_HCD_XHCI_H

/***************************************************************************/

/***************************************************************************/

struct _XHCI
{
	struct XHCI_State * State;
};

#define XHCI_RING_TRBS 256

struct XHCI_TRB
{
	U32 Parameter_Low;
	U32 Parameter_High;
	U32 Status;
	U32 Control;
};

struct XHCI_Ring
{
	struct XHCI_TRB * TRBs;
	U32 Physical;
	U32 Enqueue;
	U32 Dequeue;
	U32 Cycle;
	U32 Consumer_Cycle;
};

struct XHCI_Buffer
{
	struct XHCI_Buffer * Next;
	PTR Memory;
	U32 Physical;
	U32 Offset;
	U32 Length;
	U32 Actual;
	U32 TRB_Physical;
};

struct XHCI_Slot
{
	USB2_ID Function_ID;
	USB2_ID Parent_ID;
	PTR Input;
	PTR Output;
	U32 Input_Physical;
	U32 Output_Physical;
	U32 Slot_ID;
	U32 Address;
	U32 Speed;
	U32 Context_Entries;
	U32 EP0_Packet;
	U32 Hub_Configured;
	struct XHCI_Ring EndPoints[32];
};

struct XHCI_State
{
	U32 Capability_Length;
	U32 Operational;
	U32 Runtime;
	U32 Doorbells;
	U32 Context_Size;
	U32 Max_Slots;
	U32 Max_Ports;
	U32 Legacy_Offset;
	U32 Legacy_Claimed;
	U32 Scratchpad_Count;
	U32 Scratchpad_Physical;
	U32 Scratchpad_MemID;
	U32 DCBAA_Physical;
	U32 ERST_Physical;
	PTR DCBAA;
	PTR ERST;
	PTR Scratchpad_Array;
	PTR * Scratchpads;
	struct XHCI_Ring Commands;
	struct XHCI_Ring Events;
	U32 Command_Physical;
	U32 Command_Completion;
	U32 Command_Slot;
	U32 Command_Done;
	struct XHCI_Slot * Slots[256];
	struct USB2_Signal Signal_Event;
	U8 Port_Reset_Change[256];
};

/***************************************************************************/

extern const struct HCDFunctions XHCIFunctions;

/***************************************************************************/

#endif // INC_PRIVATE_HCD_XHCI_H
