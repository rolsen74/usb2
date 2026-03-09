
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
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
// Manager Interface
U32 _man_Obtain(		struct LibraryManagerInterface *Self );
U32 _man_Release(		struct LibraryManagerInterface *Self );
PTR _man_Open(			struct LibraryManagerInterface *Self, U32 version );
PTR _man_Close(			struct LibraryManagerInterface *Self );
PTR _man_Expunge(		struct LibraryManagerInterface *Self );
PTR _man_Init(			PTR Dummy, PTR SegList, struct ExecBase *mySysBase );

// --
// Main Interface
U32 _main_Obtain(		struct USB2_DriverIFace *Self );
U32 _main_Release(		struct USB2_DriverIFace *Self );
U32 _main_Driver_Entry(	struct USB2_DriverIFace *Self, struct USB2_DriverMessage *msg );

// --
// Device Resources Functions
S32		Resources_Init(		struct PTPBase *libBase );
void	Resources_Free(		struct PTPBase *libBase );
S32		myOpenResources(	int max_libs );
void	myCloseResources(	void );

// --
// FileSystem
U32					_fs_Handler(					void );
S32					_fs_Handler_Init(				struct PTP_FSStruct *fs );
void				_fs_Handler_Free(				struct PTP_FSStruct *fs );
void				_fs_Handler_Loop(				struct PTP_FSStruct *fs );
void				_fs_Handler__FSVector(			struct PTP_FSStruct *fs );
void				_fs_Handler__Commands(			struct PTP_FSStruct *fs );
struct FS_ObjNode *	_fs_Node_CreateDir(				struct PTP_FSStruct *fs, S32 *res2, STR oname );
void				_fs_Node_Delete(				struct PTP_FSStruct *fs, struct FS_ObjNode *node );
struct FS_ObjNode *	_fs_Node_Locate_Rel(			struct PTP_FSStruct *fs, struct FS_ObjNode **parent, struct FS_ObjLock *lock, int32 follow_links, int32 *res2, STR name );
struct FS_ObjLock * _fs_Node_NewLock(				struct PTP_FSStruct *fs, struct FS_ObjNode *objnode, int32 *res2, int32 access );
int32				_fs_Lock_Free(					struct PTP_FSStruct *fs, struct FS_ObjLock *lock );
void				_fs_Recursive_Change_Update(	struct PTP_FSStruct *fs, struct FS_ObjNode *node );

int32				_fs__FSDeviceInfoData(			struct FSVP *vp, int32 *res2, struct InfoData *id );
struct Lock *		_fs__FSDupLock(					struct FSVP *vp, int32 *res2, struct Lock *lockin );
struct Lock *		_fs__FSDupLockFromFH(			struct FSVP *vp, int32 *res2, struct FileHandle *file );
int32				_fs__FSExamineDir(				struct FSVP *vp, int32 *res2, struct PRIVATE_ExamineDirContext *ctx );
struct ExamineData *_fs__FSExamineFile(				struct FSVP *vp, int32 *res2, struct FileHandle *fh );
struct ExamineData *_fs__FSExamineLock(				struct FSVP *vp, int32 *res2, struct Lock *objLock );
struct Lock *		_fs__FSLock(					struct FSVP *vp, int32 *res2, struct Lock *rel_lock, CONST_STRPTR name, int32 mode );
struct Lock *		_fs__FSParentDir(				struct FSVP *vp, int32 *res2, struct Lock *dirlock );
struct Lock *		_fs__FSParentOfFH(				struct FSVP *vp, int32 *res2, struct FileHandle *file );
int32				_fs__FSUnLock(					struct FSVP *vp, int32 *res2, struct Lock *lock );
int32				_fs__FSVolumeInfoData(			struct FSVP *vp, int32 *res2, struct InfoData *info );

// --
// Master
void	Master_Entry(		void );
S32		Master_Init(		struct intern *in );
void	Master_Free(		struct intern *in );
void	Master_Loop(		struct intern *in );

#endif