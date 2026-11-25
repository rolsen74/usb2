
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef WIFI_PROFILE_EDITOR_H
#define WIFI_PROFILE_EDITOR_H

#include <proto/intuition.h>
#include <interfaces/listbrowser.h>

enum EditorClass
{
	EDITOR_WINDOW,
	EDITOR_LAYOUT,
	EDITOR_LIST,
	EDITOR_TABS,
	EDITOR_BUTTON,
	EDITOR_STRING,
	EDITOR_CHECK,
	EDITOR_CHOOSER,
	EDITOR_LABEL,
	EDITOR_CLASSES
};

enum EditorButton
{
	EDITOR_USE = 2,
	EDITOR_CANCEL,
	EDITOR_NETWORKS,
	EDITOR_ADAPTERS,
	EDITOR_ADD_NETWORK,
	EDITOR_REMOVE_NETWORK,
	EDITOR_EDIT_NETWORK,
	EDITOR_ADD_ADAPTER,
	EDITOR_REMOVE_ADAPTER,
	EDITOR_EDIT_ADAPTER
};

struct ProfileEditor
{
	Class *Classes[EDITOR_CLASSES];
	Class *PageClass;
	struct ListBrowserIFace *ListBrowser;
	struct Screen *Screen;
	U32 GadgetBase;
	struct ProfileDatabase Database;
};

Object *WiFi_AddEditorChild( Object *layout, Object *child, const struct TagItem *tags );
Object *WiFi_EditorButton( const struct ProfileEditor *editor, U32 id, const char *text );
S32 WiFi_EditProfileEntry( struct ProfileEditor *editor, S32 adapter, APTR entry );

#endif
