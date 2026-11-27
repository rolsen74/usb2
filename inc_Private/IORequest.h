
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_REQUEST_H
#define INC_PRIVATE_REQUEST_H

/***************************************************************************/

struct __ohci
{
	struct OHCI_ED *  ED_Header;
	struct OHCI_ITD * IsoHead;
	struct OHCI_ED *  Start_Header;
	U32				  Start_Slot;
	U32				  IsoNextFrame;
	U32				  IsoLastFrame;
	U32				  IsoFirstFrame;
	U32				  PeriodicMask;
	U32				  PeriodicCost;
};

struct __uhci
{
	struct UHCI_QH * QH_Header;
};

struct __ehci
{
	struct EHCI_QH *  QH_Header;
	struct EHCI_ITD * IsoHead;
	struct EHCI_QH *  Start_Header;
	U32				  Start_Slot;
	U32				  IsoRetire;
	U32				  IsoFirstFrame;
	U32				  IsoNextFrame;
	U32				  IsoLastFrame;
	U32				  InterruptRetire;
};

struct __xhci
{
	struct XHCI_Slot *	 Slot;
	U32					 EndPoint;
	U32					 First_TRB;
	U32					 Last_TRB;
	U32					 Actual;
	U32					 Completion;
	U32					 Finished;
	U32					 Short_Packet;
	U32					 Iso_Lost;
	struct XHCI_Buffer * Buffers;
};

/*[ Real Private Struct ]***************************************************/

enum IORStat
{
	IORS_Unset,		 // 0 : hmm Error?
	IORS_User,		 // 1 : User controlled
	IORS_HCD,		 // 2 : Gived to HCD, processing it
	IORS_HCD_Queued, // 3 : HCD have queued it
	IORS_HCD_Active, // 4 : HCD have give it to Hardware
	IORS_HCD_Pending,
};

struct RealRequest
{
	struct USB2_IORequest req_Public; // At the moment must be first, todo: do same as EPResource or ?

	// --
	struct USB2_Node req_LiveNode;
	struct Message	 req_AbortMessage;
	U32				 req_Submission;
	U32				 req_SubmitEpoch;
	U32				 req_ExpectedEpoch;
	U8				 req_CheckEpoch;
	U32				 req_AbortSubmission;
	U8				 req_AbortQueued;
	U8				 req_AbortRequested;
	U32				 req_StructID;
	S32				 req_Locks;
	U16				 req_Detach;
	U16				 req_FreeMe;
	//	struct USB2_TaskNode *			req_Task;
	//	struct USB2_ASync *				req_ASync;
	// --

	struct RealFunctionNode *	  req_Function;
	struct USB2_InterfaceHeader * req_AltHeader;
	struct USB2_EndPointNode *	  req_DestallTarget;
	struct RealEndPointResource * req_Resource;
	U8							  req_AltApplied;
	U8							  req_AltStarted;
	U8							  req_SetConfig;
	struct USB2_EndPointNode *	  req_EndPoint;
	struct USB2_MsgPort			  req_MsgPort;
	struct TimeRequest			  req_TimerIOReq;
	enum IORStat				  req_PublicStat;

	U8 req_TimerAdded;

#if 0
//	U8								rr_HCDControlled;			// HCD has control, need to Reply it
//	U8								rr_Pad[3];

	U32								rr_DMASize;
	U32								rr_DMAFlags;

	U8								rr_MsgPortUsed;
	U8								rr_HCDActive;				// The HCD has made it Active
	U8								rr_Pad[3];

#ifdef DO_DEBUG
	STR								rr_TaskName;
#endif

#endif

	union
	{
		struct __ohci OHCI;
		struct __uhci UHCI;
		struct __ehci EHCI;
		struct __xhci XHCI;
	} req_HCD;
};

const struct USB2_InterfaceHeader *USB2_IORequest_Interface( const struct RealRequest *ioreq );
void USB2_IORequest_Describe( const struct RealRequest *ioreq, const struct USB2_InterfaceHeader *interface,
	struct USB2_IORequestInfo *info );
U32 USB2_IORequest_Snapshot( struct USBBase *usbbase, struct USB2_ActiveIORequest *buffer, U32 bytes );

/***************************************************************************/

#endif // INC_PRIVATE_REQUEST_H
