
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
// ExportTree.c
S32		Save_Device_Tree( const char *path );
void	Request_Device_Tree_Save( struct Window *window );

// --
// FktNode.c
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
void	Build_Device_Properties( struct List *list, const struct USB2_FktNode *node );

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
S32		USB_Setup( void );
void	USB_Cleanup( void );
STR		GetClassName( U32 class, U32 subclass, U32 protocol );


// --

#endif
