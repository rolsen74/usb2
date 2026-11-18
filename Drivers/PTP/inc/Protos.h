
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
// Extern
extern struct MsgPort *Master_MsgPort;
extern struct MsgPort *FS_CmdMsgPort;

// --
// Manager Interface
U32		_man_Obtain(		struct LibraryManagerInterface *Self );
U32		_man_Release(		struct LibraryManagerInterface *Self );
PTR		_man_Open(			struct LibraryManagerInterface *Self, U32 version );
PTR		_man_Close(			struct LibraryManagerInterface *Self );
PTR		_man_Expunge(		struct LibraryManagerInterface *Self );
PTR		_man_Init(			PTR Dummy, PTR SegList, struct ExecBase *mySysBase );

// --
// Device Resources Functions
S32		Resources_Init(		struct PTPBase *libBase );
void	Resources_Free(		struct PTPBase *libBase );
S32		myOpenResources(	int max_libs );
void	myCloseResources(	void );

// --
// FileSystem
U32					_fs_Handler(					void );
S32					_fs_Handler_Init(				struct FS_Struct *fs );
void				_fs_Handler_Free(				struct FS_Struct *fs );
void				_fs_Handler_Loop(				struct FS_Struct *fs );
void				_fs_Handler__FSVector(			struct FS_Struct *fs );
void				_fs_Handler__Commands(			struct FS_Struct *fs );

// Helper Functions
struct FS_ObjNode *_fs_Node_CreateDir( struct FS_Struct *fs, int32 *res2, STR name );
void _fs_Node_Delete( struct FS_Struct *fs, struct FS_ObjNode *node );
struct FS_ObjNode *_fs_Node_Locate_Rel( struct FS_Struct *fs, int32 *res2, struct FS_ObjLock *lock, CONST_STRPTR name );
struct FS_ObjNode *_fs_Node_Find( struct FS_ObjNode *node, CONST_STRPTR name, U32 length );
int32 _fs_Node_Load( struct FS_Struct *fs, struct FS_ObjNode *node );
struct ExamineData *_fs_ExamineNode( struct FS_ObjNode *node, struct PRIVATE_ExamineDirContext *context );
struct FS_ObjLock *_fs_Lock_New_From_Node( struct FS_Struct *fs, struct FS_ObjNode *node, int32 *res2, int32 access );
int32 _fs_Lock_Free( struct FS_Struct *fs, struct FS_ObjLock *lock );
int32 _fs_FileCache_Create( struct FS_ObjLock *lock, int32 mode );
struct FS_ObjLock *_fs_File_Create( struct FS_Struct *fs, int32 *res2, struct FS_ObjLock *rel, CONST_STRPTR name );
int32 _fs_File_Upload( struct FS_ObjLock *lock );
int32 _fs_FileCache( struct FS_ObjLock *lock );
void _fs_FileCache_Close( struct FS_ObjLock *lock );
int32 _fs__FSOpen( struct FSVP *vp, int32 *res2, struct FileHandle *file, struct Lock *rel, CONST_STRPTR name, int32 mode );
int32 _fs__FSOpenFromLock( struct FSVP *vp, int32 *res2, struct FileHandle *file, struct Lock *lock );
int32 _fs__FSClose( struct FSVP *vp, int32 *res2, struct FileHandle *file );
int32 _fs__FSWrite( struct FSVP *vp, int32 *res2, struct FileHandle *file, STR buffer, int32 count );
int32 _fs__FSRead( struct FSVP *vp, int32 *res2, struct FileHandle *file, STR buffer, int32 count );
int32 _fs__FSChangeFilePosition( struct FSVP *vp, int32 *res2, struct FileHandle *file, int32 mode, S64 position );
S64 _fs__FSGetFilePosition( struct FSVP *vp, int32 *res2, struct FileHandle *file );
S64 _fs__FSGetFileSize( struct FSVP *vp, int32 *res2, struct FileHandle *file );

// --

// Vector Entries .. Use Semaphore Locks
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
S32		Master_Init(		struct M_Struct *ms );
void	Master_Free(		struct M_Struct *ms );
void	Master_Loop(		struct M_Struct *ms );

// --
// USB
U32		_usb_Obtain(		struct USB2DriverIFace *Self );
U32		_usb_Release(		struct USB2DriverIFace *Self );
U32		_usb_Entry(			struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
U32		_usb_Init(			struct USB_Struct *us );
void	_usb_Free(			struct USB_Struct *us );
void	_usb_Loop(			struct USB_Struct *us );


void _usb_Loop_Stack( struct USB_Struct *us );
void _usb_QueryUpload( struct USB_Struct *us );
S32 _usb_SendData( struct USB_Struct *us, struct CommandMessage *request, U32 transaction );
S32 _usb_WriteBulk( struct USB_Struct *us, U32 length, S32 final );
void _usb_Transaction( struct USB_Struct *us, struct CommandMessage *request );
int32 _fs_Camera_Request( struct CameraInfoNode *camera, struct CommandMessage *request );

#endif
