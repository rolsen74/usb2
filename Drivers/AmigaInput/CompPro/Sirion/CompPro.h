
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef __COMPPRO_H__
#define __COMPPRO_H__

// --

#define __USE_INLINE__
#define __USE_BASETYPE__

#include <proto/amigainput.h>
#include <proto/amigainput_driver.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usbfd.h>
#include <proto/usbsys.h>
#include <proto/usbresource.h>

#include <usb/usb.h>
#include <usb/system.h>
#include <usb/devclasses.h>

#include <inttypes.h>
#include <string.h>

// --

#ifndef U8
typedef uint8_t U8;
#endif

#ifndef U16
typedef uint16_t U16;
#endif

#ifndef U32
typedef uint32_t U32;
#endif

#ifndef U64
typedef uint64_t U64;
#endif

#ifndef S8
typedef int8_t S8;
#endif

#ifndef S16
typedef int16_t S16;
#endif

#ifndef S32
typedef int32_t S32;
#endif

#ifndef S64
typedef int64_t S64;
#endif

#ifndef PTR
typedef void* PTR;
#endif

#ifndef STR
typedef char* STR;
#endif

#ifndef LE_SWAP16
#define LE_SWAP16(x)	( (( (x) & 0x00ff ) << 8 ) | (( (x) & 0xff00 ) >> 8 ) )
#endif

#ifndef BE_SWAP16
#define BE_SWAP16(x)	(x)
#endif

// --

#include "AmigaInput/_AmigaInput.h"

// --

#define LIBNAME			"CompPro.usbfd"
#define VERSION			1
#define REVISION		1
#define DATE			"19.10.2008"
#define VERS			"CompPro.usbfd 1.1"
#define VSTRING			"CompPro.usbfd 1.1 (19.10.2008)\r\n"
#define VERSTAG			"\0$VER: CompPro.usbfd 1.1 (19.10.2008)"

// --

#define MAX_REPORT_FIELDS 64

struct HID_ReportField 
{
	U8					report_id;
	U8					usage_page;
	U16					usage;
	U8					report_size;
	U8					report_count;
	U8					type;	// 0 = Input, 1 = Output, 2 = Feature
	U8					flags;
};

struct libBase
{
	struct Library		lib_Base;
	U16					lib_Pad;
	PTR					lib_SegList;
	PTR					lib_fdkey;
};

struct USB_Controller
{
	STR					name;
	U32					vendor;
	U32					product;
	U32					type;
};

#pragma pack(1)

struct HID_DescItem
{
	U8					Type;			// Descriptor Type
	U16					Length;			// Length of Descriptor
};

struct HID_Desc
{
	U8					Length;
	U8					Type;
	U16					Version;		// BCD
	U8					Country;
	U8					NumDescs;		// Number of Descriptors
	struct HID_DescItem	Descs[1];		// Descriptor(s)
};

#pragma pack(0)

struct USB_Driver
{
	// Uhh can I make those Global??
	struct Library *			USBSysBase;
	struct USBSysIFace *		IUSBSys;

	// --
	struct AINSubIFace *		AI_IFace;
	PTR							AI_Driver;
	struct libBase *			libBase;
	U32							Running;

	U32							Report_Size;
	PTR							Report_Buffer;
	struct HID_ReportField		Report_Fields[MAX_REPORT_FIELDS];

	// --
	struct UsbRawFunction *		USB_RawFunction;
	PTR							USB_Function;			// Claimed
	U32							USB_Function_Bit;
	struct MsgPort *			USB_Function_MsgPort;
	struct UsbInterface *   	USB_Interface;
	U32							USB_Interface_Nr;
	struct USBBusIntDsc *		USB_Interface_Dsc;
	struct USBBusCfgDsc *		USB_Config_Descs;		// Root Config
	struct USBBusCfgDsc *		USB_Config_Dsc;			// Our Active Cfg (may change)
	struct HID_Desc *			USB_HID_Dsc;
	struct IORequest *			USB_IOReq;

	// --
	struct UsbEndPoint *		USB_Ctrl_EP_Ref;
	U32							USB_Ctrl_MP_Bit;
	struct MsgPort *			USB_Ctrl_MP;
	struct USBIOReq *			USB_Ctrl_IOReq;

	// --
	struct USBBusEPDsc *		USB_IntIn_EP_Dsc;
	struct UsbEndPoint *		USB_IntIn_EP_Ref;
	U32							USB_IntIn_MP_Bit;
	struct MsgPort *			USB_IntIn_MP;
	struct USBIOReq *			USB_IntIn_IOReq;
	U32							USB_IntIn_BufSize;
	PTR							USB_IntIn_Buffer;

	// --
	struct USBBusEPDsc *		USB_IntOut_EP_Dsc;
	struct UsbEndPoint *		USB_IntOut_EP_Ref;
	U32							USB_IntOut_MP_Bit;
	struct MsgPort *			USB_IntOut_MP;
	struct USBIOReq *			USB_IntOut_IOReq;
	U32							USB_IntOut_BufSize;
	PTR							USB_IntOut_Buffer;

	// --

};

// --

PTR		_manager_Init(			struct libBase *libBase, PTR seglist, struct ExecIFace *myIExec );
U32		_manager_Obtain(		struct LibraryManagerInterface *Self );
U32		_manager_Release(		struct LibraryManagerInterface *Self );
PTR		_manager_Open(			struct LibraryManagerInterface *Self, U32 version );
PTR		_manager_Close(			struct LibraryManagerInterface *Self );
PTR		_manager_Expunge(		struct LibraryManagerInterface *Self );

U32		_main_Obtain(			struct USBFDIFace *Self );
U32		_main_Release(			struct USBFDIFace *Self );
U32		_main_GetAttrsA(		struct USBFDIFace *Self, struct TagItem *taglist );
U32		_main_GetAttrs(			struct USBFDIFace *Self, ... );
S32		_main_RunFunction(		struct USBFDIFace *Self, struct USBFDStartupMsg *msg );
S32		_main_RunInterface( 	struct USBFDIFace *Self, struct USBFDStartupMsg *msg );

S32		_AI_CleanUp(			struct AINSubIFace *Self, AIN_DeviceID ID );
S32		_AI_HandleSettings(		struct AINSubIFace *Self, AIN_DeviceID ID, AIN_DriverSettings *settings, BOOL load, BOOL use );
S32		_AI_InitDriver(			struct AINSubIFace *Self );
U32		_AI_Obtain(				struct AINSubIFace *Self );
S32		_AI_Read_KBD(			struct AINSubIFace *Self, AIN_DeviceID ID, STR puffer );
void	_AI_Read(				struct AINSubIFace *Self, AIN_DeviceID ID );
U32		_AI_Release(			struct AINSubIFace *Self );
void	_AI_SetCalibrateMode(	struct AINSubIFace *Self, AIN_DeviceID ID, BOOL Mode );
U32		_AI_SetEventMode(		struct AINSubIFace *Self, AIN_DeviceID ID, U32 MOD );
U32		_AI_TestUnit(			struct AINSubIFace *Self, AIN_DeviceID ID );

S32		Driver_Function(		struct USBFDIFace *Self, struct USBFDStartupMsg *msg, U32 type );
S32		Driver_Init(			struct USB_Driver *in );
void	Driver_Free(			struct USB_Driver *in );
void	Driver_Main(			struct USB_Driver *in );
void	Driver_Handle_Stack(	struct USB_Driver *in );
S32		Driver_Report_Get(		struct USB_Driver *in );
S32		Driver_Report_Parse(	struct USB_Driver *in );
S32		Driver_AI_Add(			struct USB_Driver *in );
void	Driver_AI_Free(			struct USB_Driver *in );

S32		Resources_Setup(		struct libBase *libBase );
void	Resources_Cleanup(		struct libBase *libBase );

// --

#endif
