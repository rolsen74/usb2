
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

#ifndef DRIVERS_HID_HID_H
#define DRIVERS_HID_HID_H

/***************************************************************************/

#define HID_IOReqCount				3

// --

enum HID_Driver_Type
{
	HID_DType_Unset,
	HID_DType_Keyboard,
	HID_DType_Mouse,
	HID_DType_Other
};

enum HID_Driver_Mode
{
	HID_DMode_Unset,
	HID_DMode_Boot,
	HID_DMode_Report
};

// --

#pragma pack(1)

struct USB2_BootMouse		// 3 bytes
{
	U8		Buttons;
	S8		X;
	S8		Y;
//	S8		Wheel;			// non standard, can really be anything
};

#define BOOTMOUSE_BUTTONS_LEFT			0x01U
#define BOOTMOUSE_BUTTONS_RIGHT			0x02U
#define BOOTMOUSE_BUTTONS_MIDDLE		0x04U
#define BOOTMOUSE_BUTTONS_4TH			0x08U
#define BOOTMOUSE_BUTTONS_5TH			0x10U
#define BOOTMOUSE_BUTTONS_MASK			0x1FU

struct USB2_BootKeyboard	// 8 bytes
{
	U8		Modifier;
	U8		Reserved;
	U8		Keycode[6];
};

#pragma pack(0)

/***************************************************************************/

struct _Boot_Keyboard
{
	struct USB2_BootKeyboard		KeyboardData;
	struct Preferences				Prefs_Buffer;
	struct TimeVal					Time_Thresh;
	struct TimeVal					Time_Repeat;
	struct TimeRequest				Timer_IOReq;
	struct USB2_MsgPort				Timer_MsgPort;
	U32		Timer_Added;
	S16		PrevCode[2];
	U16		PrevQual[2];
	S16		Key_Last;		// We want this key to become active
	S16		Key_Repeat;		// Active key
};

struct _Boot_Mouse
{
	U8		Buttons;
};

struct KeyboardMask
{
	U8		Mask1;
	U8		Mask2;
	U8		KeyCode;
	U8		Pad;
};

struct intern
{
	#ifdef DO_DEBUG
	U32								StructID;
	#endif

	enum HID_Driver_Type			Driver_Type;
	enum HID_Driver_Mode			Driver_Mode;

	struct USB2_EPResource *		Res_Interrupt;
	struct USB2_EPResource *		Res_Control;
	struct USB2_DriverMessage *		StartMsg;
	struct RealRegister *			Register;

	struct USB2_MsgPort				Input_MsgPort;
	struct IOStdReq					Input_IOReq;
	struct InputEvent				Input_Event;

	U32		Qualifiers;
	U32		ErrorCnt;
	U32		Running;

	union
	{
		struct _Boot_Keyboard		Boot_Keyboard;
		struct _Boot_Mouse			Boot_Mouse;
	} 								Type;

//	struct TimeRequest				Delay_IOReq;
//	struct USB2_MsgPort				Delay_MsgPort;
//	struct RealFunctionNode **		PortFunctions;
//	struct RealFunctionNode *		Function;
//	struct USB2_HCDNode *			HCDNode;
//	U32		Ports;
};

	#if 0

#pragma pack(1)

struct USB2_BootMouse
{
    uint8   Buttons;
    int8    X;
    int8    Y;
};

struct USB2_BootKeyboard
{
	uint8   Modifier;
    uint8   reserved;
	uint8   Keycode[USBKBD_KEYCODEFIELDS];
};

#pragma pack(0)

struct mouse
{
	uint16							Buttons;		// Old State
};

struct keyboard
{
	struct USB2_BootKeyboard			 KeyboardData;
	struct Preferences				PrefsBuffer;

	struct TimeVal					KbdThreshTime;
	struct TimeVal					KbdRepeatTime;
	uint8 *							KbdReport;
	uint8							KbdPrevCode[2];
	uint8							KbdPrevQual[2];
	uint8							KbdRepeatKey;
	uint8							KbdPad[3];
};


struct hidintern
{
	struct USB2_StartupMessage *		 StartMsg;
	struct RealFunctionNode *		 Function;
	APTR							HCDNode;

	struct TimeRequest			    TimerIOReq;
	struct USB2_MsgPort				 TimerMsgPort;
	U32						    TimerMsgPortBit;
	int							TimerUsed;

	struct IOStdReq				    InputIOReq;
	struct USB2_MsgPort				 InputMsgPort;
	U32					        InputMsgPortBit;
	struct InputEvent       	    InputEvent;
	APTR							InputBuffer;
	U32							InputBufferSize;

	struct RealRegister *		    Register;

	struct USB2_EPResource *    Control_Resource;
	struct USB2_EPResource *    Interrupt_Resource;
	struct USB2_InterfaceDescriptor * Interface_Descriptor;

	struct USB2_Header				 Reports;


	APTR							ReportBuffer;
	U32							ReportSize;
	U32							DriverType;

	// --

	uint16						    ErrorCount;
	uint16						    Qualifiers;

	int							UsesReportID;
	int						    ReportMode;
//--	U32							FreeSignals;
	S32							retval;

	union
	{
		struct mouse    			Mouse;
		struct keyboard 			Keyboard;
//		  struct report   			  Report;
	}								Mode;
};

struct KeyboardMask
{
	uint8	Mask1;
	uint8	Mask2;
	uint8	KeyCode;
	uint8	Pad;
};

	#endif

extern const struct KeyboardMask KeyboardData[];
extern const S16 USBNumPadMap[256];
extern const S16 USBKeyMap1[256];
extern const S16 USBKeyMap2[256];

SEC_CODE void	HID_SendRawKey( struct USBBase *usbbase, struct intern *in, U16 rawkey );
SEC_CODE void	HID_Boot_Mouse_Buttons( struct USBBase *usbbase, struct intern *in, struct USB2_BootMouse *report );
SEC_CODE void	HID_Boot_Handle_Stack( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HID_Boot( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HID_Free( struct USBBase *usbbase, struct intern *in );
SEC_CODE S32	HID_Init( struct USBBase *usbbase, struct intern *in );

SEC_CODE void	HID_Boot_DoKeyboardTimer( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HID_Boot_Keyboard( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HID_Boot_Mouse( struct USBBase *usbbase, struct intern *in );

/***************************************************************************/

#endif // DRIVERS_HUB_HUB_H
