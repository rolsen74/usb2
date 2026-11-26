
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_PROTOS_H
#define INC_PROTOS_H

// --
// DescribeDescriptors.c
typedef void ( *DescriptorRow )( PTR target, U32 level, S32 heading, const char *field,
	const char *size, const char *value, const char *description );
void Describe_Descriptors( const struct USB2_FktNode *node, DescriptorRow emit, PTR target );

// --
// DescriptorImage.c
S32 Open_Descriptor_Image_Class( void );
void Close_Descriptor_Image_Class( void );
Object *New_Descriptor_Image( struct TextFont *font, const struct DrawInfo *draw );
S32 Append_Descriptor_Image( Object *object, const char *field, const char *size, const char *value );

// --
// DeviceProperties.c
void Build_Device_Properties( struct List *list, const struct USB2_FktNode *node, struct Window *window );
void Free_Device_Properties( struct List *list );

// --
// DriverTitles.c
const char *Read_Driver_Titles( USB2_ID id, U32 tag, STR buffer, U32 capacity, STR *allocated );

// --
// ExportTree.c
S32		Save_Device_Tree( const char *path );
void	Request_Device_Tree_Save( struct Window *window );

// --
// FktNode.c
void FktNode_Format_Interface( const struct USB2_FktNode *node, STR label, U32 size );
struct USB2_FktNode *	FktNode_Alloc( U32 type, USB2_ID id );
void					FktNode_Delete( struct USB2_FktNode *n );
struct USB2_FktNode *	FktNode_Find( struct List *l, USB2_ID id );
void					FktNode_Reset( struct USB2_FktNode *n );

// --
// MainScreen.c
S32		Main_OpenScreen( void );
void	Main_CloseScreen( void );
S32		Request_Screen_Mode( struct Window *window, struct ScreenPrefs *prefs );

// --
// MainWindow.c
S32		Main_OpenWindow( void );
void	Main_CloseWindow( void );
S32		Main_BuildWindow( void );
void	Main_DestoryWindow( void );
void	Main_HandleWindow( void );
void	Build_Device_Tree( void );
void	Show_IO_Capture( struct List *events );
void	Update_Window_Timer( void );

// --
// IOCapture.c
#define IO_CAPTURE_COLUMNS 6
extern const char *const IOCaptureTitles[IO_CAPTURE_COLUMNS];
extern const char *const ActiveIOTitles[IO_CAPTURE_COLUMNS];
void Measure_IO_Capture( struct List *list, struct RastPort *rp, const char *const *titles,
	U32 widths[IO_CAPTURE_COLUMNS] );
U32 Append_IO_Capture( struct List *list, const struct USB2_NotifyMessage *msg, struct RastPort *rp,
	U32 widths[IO_CAPTURE_COLUMNS] );
const struct USB2_NotifyMessage *Get_IO_Capture( struct Node *row );
void Free_IO_Capture( struct List *list );
void Free_IO_Request_Rows( struct List *list );
void Build_IO_Request_Details( struct List *list, const struct USB2_NotifyMessage *entry );
U32 Build_Active_IO_Requests( struct List *list, struct RastPort *rp, U32 widths[IO_CAPTURE_COLUMNS],
	U32 sortColumn, S32 sortReverse );

// --
// WindowTimer.c
S32 Start_Window_Timer( void );
void Stop_Window_Timer( void );
void Handle_Window_Timer( void );

// --
// CaptureFilter.c
S32 Capture_Is_Checked( const struct USB2_FktNode *node );
S32 Capture_Set_Checked( struct USB2_FktNode *node, S32 checked );
S32 Capture_Is_Running( void );
S32 Capture_Set_Running( S32 running );

// --
// Prefs.c
void Prefs_Load( void );
void Prefs_OpenWindow( struct Window *parent );
void Prefs_CloseWindow( void );
void Prefs_HandleWindow( void );

// --
// Notify.c
void	Handle_Notify( void );

// --
// Resources.c
S32		myOpenResources( int );
void	myCloseResources( void );

// --
// USB.c
S32		USB_Setup( S32 capture );
S32		USB_Has_Capture( void );
S32		USB_Has_Active_IO( void );
void	USB_Get_Stack_Version( U16 *version, U16 *revision );
S32		USB_Set_Capture_Filter( const struct USB2_IOCaptureTarget *targets, U32 count );
void	USB_Cleanup( void );
STR		GetClassName( U32 class, U32 subclass, U32 protocol );


// --

#endif
