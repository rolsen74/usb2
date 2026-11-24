
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
extern struct MsgPort *FS_CmdMsgPort;
extern struct SignalSemaphore FS_PortGate;
extern U32 FS_Users;
extern S32 FS_Closing;

void _fs_Init_Acquire( void );
S32 _fs_Acquire( void );
void _fs_Release( void );
S32 _fs_Start( S32 restart );

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
S32 _fs_Handler_Shutdown( struct FS_Struct *fs );
void				_fs_Handler__FSVector(			struct FS_Struct *fs );
void				_fs_Handler__Commands(			struct FS_Struct *fs );

// Helper Functions
struct FS_ObjNode *_fs_Node_CreateDir( struct FS_Struct *fs, S32 *res2, STR name );
void _fs_Node_Delete( struct FS_Struct *fs, struct FS_ObjNode *node );
struct FS_ObjNode *_fs_Node_Locate_Rel( struct FS_Struct *fs, S32 *res2, struct FS_ObjLock *lock, STR name );
struct FS_ObjNode *_fs_Node_Find( struct FS_ObjNode *node, STR name, U32 length );
S32 _fs_Node_Load( struct FS_Struct *fs, struct FS_ObjNode *node );
struct ExamineData *_fs_ExamineNode( struct FS_ObjNode *node, struct PRIVATE_ExamineDirContext *context );
struct FS_ObjLock *_fs_Lock_New_From_Node( struct FS_Struct *fs, struct FS_ObjNode *node, S32 *res2, S32 access );
S32 _fs_Lock_Free( struct FS_Struct *fs, struct FS_ObjLock *lock );
S32 _fs_FileCache_Create( struct FS_ObjLock *lock, S32 mode );
struct FS_ObjLock *_fs_File_Create( struct FS_Struct *fs, S32 *res2, struct FS_ObjLock *rel, STR name );
S32 _fs_File_Upload( struct FS_Struct *fs, struct FS_ObjLock *lock );
S32 _fs_FileCache( struct FS_ObjLock *lock );
void _fs_FileCache_Close( struct FS_ObjLock *lock );
LONG _fs__FSOpen( struct FSVP *vp, LONG *res2, struct FileHandle *file, struct Lock *rel, const char *filename, LONG mode );
LONG _fs__FSOpenFromLock( struct FSVP *vp, LONG *res2, struct FileHandle *file, struct Lock *lock );
LONG _fs__FSClose( struct FSVP *vp, LONG *res2, struct FileHandle *file );
LONG _fs__FSWrite( struct FSVP *vp, LONG *res2, struct FileHandle *file, STR buffer, LONG count );
LONG _fs__FSRead( struct FSVP *vp, LONG *res2, struct FileHandle *file, STR buffer, LONG count );
LONG _fs__FSChangeFilePosition( struct FSVP *vp, LONG *res2, struct FileHandle *file, LONG mode, S64 position );
S64 _fs__FSGetFilePosition( struct FSVP *vp, LONG *res2, struct FileHandle *file );
S64 _fs__FSGetFileSize( struct FSVP *vp, LONG *res2, struct FileHandle *file );

// --

// Vector Entries .. Use Semaphore Locks
LONG				_fs__FSDeviceInfoData(			struct FSVP *vp, LONG *res2, struct InfoData *id );
struct Lock *		_fs__FSDupLock(					struct FSVP *vp, LONG *res2, struct Lock *lockin );
struct Lock *		_fs__FSDupLockFromFH(			struct FSVP *vp, LONG *res2, struct FileHandle *file );
LONG				_fs__FSExamineDir(				struct FSVP *vp, LONG *res2, struct PRIVATE_ExamineDirContext *ctx );
struct ExamineData *_fs__FSExamineFile(				struct FSVP *vp, LONG *res2, struct FileHandle *fh );
struct ExamineData *_fs__FSExamineLock(				struct FSVP *vp, LONG *res2, struct Lock *objLock );
struct Lock *		_fs__FSLock(					struct FSVP *vp, LONG *res2, struct Lock *rel_lock, const char *name, LONG mode );
struct Lock *		_fs__FSParentDir(				struct FSVP *vp, LONG *res2, struct Lock *dirlock );
struct Lock *		_fs__FSParentOfFH(				struct FSVP *vp, LONG *res2, struct FileHandle *file );
LONG				_fs__FSUnLock(					struct FSVP *vp, LONG *res2, struct Lock *lock );
LONG				_fs__FSVolumeInfoData(			struct FSVP *vp, LONG *res2, struct InfoData *info );

// --
// Master
void	Master_Entry(		void );
void Master_Exit( int32 result, int32 data );

// --
// USB
U32		_usb_Obtain(		struct USB2DriverIFace *Self );
U32		_usb_Release(		struct USB2DriverIFace *Self );
U32		_usb_Entry(			struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
U32		_usb_Init(			struct USB_Struct *us );
void	_usb_Free(			struct USB_Struct *us );
void	_usb_Loop(			struct USB_Struct *us );


void _usb_Loop_Stack( struct USB_Struct *us );
S32 _usb_Read_Events( struct USB_Struct *us, struct USB2_IORequest *io );
void _usb_Query_Device_Info( struct USB_Struct *us );
S32 _usb_MTP_Interface( struct USB_Struct *us );
S32 _usb_SendData( struct USB_Struct *us, struct CommandMessage *request, U32 transaction );
S32 _usb_Recover( struct USB_Struct *us );
S32 _usb_WriteBulk( struct USB_Struct *us, U32 length, S32 final );
void _usb_Transaction( struct USB_Struct *us, struct CommandMessage *request );
void _usb_Service_Events( struct USB_Struct *us );
S32 _usb_Check_Fence( struct USB_Struct *us, struct CommandMessage *request );
void _usb_Invalidate_Storage( struct USB_Struct *us, S32 event_loss );
S32 _fs_Camera_Request_Node( struct FS_ObjNode *node, struct CommandMessage *request );
S32 _fs_Camera_Fence( struct FS_ObjNode *node );
S32 _fs_Camera_Request( struct CameraInfoNode *camera, struct CommandMessage *request );
S32 _fs_Store_Removed( U32 storage, const U32 *removals, U32 count );
S32 _fs_Storage_Load( struct FS_Struct *fs, struct FS_ObjNode *node );
S32 _fs_Node_Online( struct FS_ObjNode *node );
STR _fs_Camera_Name( struct FS_Struct *fs, struct CameraInfoNode *camera );
STR _fs_Storage_Name( struct FS_ObjNode *parent, U32 id );
STR _fs_Unique_Name( struct FS_ObjNode *parent, STR base );
U32 _ptp_Read_Name( const U8 *data, U32 remaining, STR name, U32 size );

#endif
