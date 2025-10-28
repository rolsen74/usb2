
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef INTERFACES_USB2_H
#define INTERFACES_USB2_H

/****************************************************************************/

#ifndef DEVICES_USB2_H
#include <devices/usb2.h>
#endif

/****************************************************************************/

struct USB2_IFace
{
	struct InterfaceData Data;

	U32								APICALL (*Obtain)(struct USB2_IFace *Self);
	U32								APICALL (*Release)(struct USB2_IFace *Self);
	void							APICALL (*Expunge)(struct USB2_IFace *Self);
	struct Interface *				APICALL (*Clone)(struct USB2_IFace *Self);

	U32								APICALL (*USB2_Attr_Get)(struct USB2_IFace *Self, U64 id, U32 tag, PTR buffer, U32 buffsersize );
	U32								APICALL (*USB2_Attr_Set)(struct USB2_IFace *Self, U64 id, PTR storage );

	struct USB2_Function *			APICALL (*USB2_Fkt_FindTags)(struct USB2_IFace *Self, ... );
	struct USB2_Function *			APICALL (*USB2_Fkt_FindList)(struct USB2_IFace *Self, struct TagItem *taglist );

	struct USB2_Interface *			APICALL (*USB2_Ifc_FindTags)(struct USB2_IFace *Self, ... );
	struct USB2_Interface *			APICALL (*USB2_Ifc_FindList)(struct USB2_IFace *Self, struct TagItem *taglist );
	S32								APICALL (*USB2_Ifc_Claim)(struct USB2_IFace *Self, struct USB2_Register *reg, struct USB2_Interface *ih );
	S32								APICALL (*USB2_Ifc_Declaim)(struct USB2_IFace *Self, struct USB2_Register *reg, struct USB2_Interface *ih );

	struct USB2_Register *			APICALL (*USB2_Reg_RegisterTags)(struct USB2_IFace *Self, ...);
	struct USB2_Register *			APICALL (*USB2_Reg_RegisterList)(struct USB2_IFace *Self, struct TagItem *taglist );
	void							APICALL (*USB2_Reg_Unregister)(struct USB2_IFace *Self, struct USB2_Register *reg );

	struct USB2_EPResource *		APICALL (*USB2_EPRes_ObtainTags)(struct USB2_IFace *Self, struct USB2_Register *reg, ... );
	struct USB2_EPResource *		APICALL (*USB2_EPRes_ObtainList)(struct USB2_IFace *Self, struct USB2_Register *reg, struct TagItem taglist );
	void							APICALL (*USB2_EPRes_Release)(struct USB2_IFace *Self, struct USB2_EPResource *epr );
	U32								APICALL (*USB2_EPRes_Destall)(struct USB2_IFace *Self, struct USB2_EPResource *epr );
};

/***************************************************************************/

struct USB2_DriverIFace
{
	struct InterfaceData Data;

	U32								APICALL ( *Obtain )	 ( struct USB2_DriverIFace *Self );
	U32								APICALL ( *Release ) ( struct USB2_DriverIFace *Self );
	void							APICALL ( *Expunge ) ( struct USB2_DriverIFace *Self );
	struct Interface *				APICALL ( *Clone )	 ( struct USB2_DriverIFace *Self );
	U32								APICALL ( *Entry )	 ( struct USB2_DriverIFace *Self, struct USB2_DriverMessage *msg );
};

/***************************************************************************/

#if 0
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef EXEC_INTERFACES_H
#include <exec/interfaces.h>
#endif

#ifndef DEVICES_USB2_H
#include <devices/usb2.h>
#endif

#ifdef __cplusplus
#ifdef __USE_AMIGAOS_NAMESPACE__
namespace AmigaOS {
#endif
extern "C" {
#endif

struct USB2_IFace
{
	struct InterfaceData Data;

	U32								APICALL (*Obtain)(struct USB2_IFace *Self);
	U32								APICALL (*Release)(struct USB2_IFace *Self);
	void							APICALL (*Expunge)(struct USB2_IFace *Self);
	struct Interface *				APICALL (*Clone)(struct USB2_IFace *Self);
	struct USB2_IORequest *			APICALL (*AllocRequestList)(struct USB2_IFace *Self, struct TagItem * tags);
	struct USB2_IORequest *			APICALL (*AllocRequestTags)(struct USB2_IFace *Self, ...);
	void							APICALL (*FreeRequest)(struct USB2_IFace *Self, struct USB2_IORequest * ioreq);
	U32								APICALL (*DestallEndPoint)(struct USB2_IFace *Self, struct USB2_EPResource * epr);
	U32								APICALL (*GetEndPointNr)(struct USB2_IFace *Self, struct USB2_EPResource * epr);
	struct USB2_Descriptor *			APICALL (*GetNextDescriptor)(struct USB2_IFace *Self, struct USB2_Descriptor * desc);
	PTR							APICALL (*AddNotify)(struct USB2_IFace *Self, U32 type, struct MsgPort * mp);
	void APICALL (*RemNotify)(struct USB2_IFace *Self, PTR notify);
	U32 APICALL (*GetMemoryInfoList)(struct USB2_IFace *Self, U32 nr, struct TagItem * tags);
	U32 APICALL (*GetMemoryInfoTags)(struct USB2_IFace *Self, U32 nr, ...);
	U32 APICALL (*Stack_GetAttribute)(struct USB2_IFace *Self, uint64 id, U32 tag, PTR buffer, U32 buffsersize);
	U32 APICALL (*Stack_SetAttribute)(struct USB2_IFace *Self, uint64 id, PTR storage);
	U32 APICALL (*Status_GetGlobalInfoList)(struct USB2_IFace *Self, struct TagItem * tags);
	U32 APICALL (*Status_GetGlobalInfoTags)(struct USB2_IFace *Self, ...);
	struct USB2_Interface_Desc * APICALL (*GetNextInterfaceDescriptor)(struct USB2_IFace *Self, struct USB2_Register *reg, struct USB2_Interface_Desc * desc);
	U32 APICALL (*ActivateAltInterface)(struct USB2_IFace *Self, struct USB2_Register *reg, U32 ifcnr, U32 altnr);
	U32 APICALL (*ReloadSettingsList)(struct USB2_IFace *Self, struct TagItem * tags);
	U32 APICALL (*ReloadSettingsTags)(struct USB2_IFace *Self, ...);
	U32 APICALL (*SaveSettingsList)(struct USB2_IFace *Self, struct TagItem * tags);
	U32 APICALL (*SaveSettingsTags)(struct USB2_IFace *Self, ...);
	U32 APICALL (*Shutdown)(struct USB2_IFace *Self, U32 flags);
	PTR APICALL (*AllocIOBuffer)(struct USB2_IFace *Self, U32 size);
	void APICALL (*FreeIOBuffer)(struct USB2_IFace *Self, PTR buf);
};

#ifdef __cplusplus
}
#ifdef __USE_AMIGAOS_NAMESPACE__
}
#endif
#endif





#endif

#endif
