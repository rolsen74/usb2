#ifndef INTERFACES_BLUETOOTH2_H
#define INTERFACES_BLUETOOTH2_H

#include <exec/interfaces.h>
#include <exec/ports.h>
#include "Bluetooth.h"
#include <interfaces/bluetooth2prefs.h>
#include <interfaces/bluetooth2control.h>
#include <interfaces/bluetooth2share.h>
#include <interfaces/bluetooth2sharecontrol.h>

#define BLUETOOTH2_NAME "bluetooth2.device"
#define BLUETOOTH2_INTERFACE "main"
#define BLUETOOTH2_INTERFACE_VERSION 4

struct BTControllerLocation
{
	uint16_t VendorID;
	uint16_t ProductID;
	uint8_t EventEndpoint;
};

// The driver worker owns the request port and services requests on its task.
struct BTControllerProbe
{
	struct Message Message;
	struct BTControllerInfo *Info;
	struct BTHCIError *Error;
	enum BTHCIResult Result;
	struct BTInquiryResult *Inquiry;
	const struct BTPairRequest *Pair;
	struct BTBondRequest *Bond;
	struct BTRemoteName *RemoteName;
	const struct BTVisibility *Visibility;
	const uint8_t *EchoAddress;
	struct BTServiceQuery *Services;
	struct BTRFCOMMTest *RFCOMM;
	struct BTRFCOMMTransfer *Serial;
	struct BTRFCOMMFile *SerialFile;
	struct BTRFCOMMCapture *SerialCapture;
	struct BTObjectPush *ObjectPush;
	struct BTLERequest *LE;
	struct BTAudioPlayback *Audio;
	struct BTAudioPCM *PCM;
	struct BTHIDMonitor *HID;
	struct BTFileShare *Share;
};

struct Bluetooth2IFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2IFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2IFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2IFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2IFace *Self );

	enum BTHCIResult APICALL (*ResetController)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTHCIError *error );
	enum BTHCIResult APICALL (*ReadControllerInfo)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTControllerInfo *info,
		struct BTHCIError *error );

	int32_t APICALL (*AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
	void APICALL (*RemoveController)( struct Bluetooth2IFace *Self, uint32_t unit, struct MsgPort *port );
	enum BTHCIResult APICALL (*ProbeController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTControllerLocation *location, struct BTControllerInfo *info, struct BTHCIError *error );
	enum BTHCIResult APICALL (*DiscoverController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTControllerLocation *location, struct BTInquiryResult *inquiry, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Inquiry)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTInquiryResult *inquiry, struct BTHCIError *error );
	int32_t APICALL (*AddDiscoveryController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*pairing_PairController)( struct Bluetooth2IFace *Self, uint32_t unit,
		const struct BTPairRequest *pair, struct BTHCIError *error );
	enum BTHCIResult APICALL (*pairing_Pair)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, const struct BTPairRequest *pair, struct BTHCIError *error );
	int32_t APICALL (*pairing_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*bonding_BondController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTBondRequest *bond, struct BTHCIError *error );
	enum BTHCIResult APICALL (*bonding_Bond)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTBondRequest *bond, struct BTHCIError *error );
	int32_t APICALL (*bonding_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*names_ReadName)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTRemoteName *remote, struct BTHCIError *error );
	enum BTHCIResult APICALL (*names_ReadRemoteName)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTRemoteName *remote, struct BTHCIError *error );
	int32_t APICALL (*names_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*visibility_MakeVisible)( struct Bluetooth2IFace *Self, uint32_t unit,
		const struct BTVisibility *visibility, struct BTHCIError *error );
	enum BTHCIResult APICALL (*visibility_RunVisible)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, const struct BTVisibility *visibility, struct BTHCIError *error );
	int32_t APICALL (*visibility_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*l2cap_EchoController)( struct Bluetooth2IFace *Self, uint32_t unit,
		const uint8_t *address, struct BTHCIError *error );
	enum BTHCIResult APICALL (*l2cap_Echo)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, const uint8_t *address, struct BTHCIError *error );
	int32_t APICALL (*l2cap_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*sdp_BrowseController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTServiceQuery *query, struct BTHCIError *error );
	enum BTHCIResult APICALL (*sdp_Browse)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error );
	int32_t APICALL (*sdp_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*rfcomm_TestController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTRFCOMMTest *test, struct BTHCIError *error );
	enum BTHCIResult APICALL (*rfcomm_Test)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMTest *test, struct BTHCIError *error );
	int32_t APICALL (*rfcomm_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*serial_ExchangeController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
	enum BTHCIResult APICALL (*serial_Exchange)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
	int32_t APICALL (*serial_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*serialfile_SendController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTRFCOMMFile *file, struct BTHCIError *error );
	enum BTHCIResult APICALL (*serialfile_Send)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMFile *file, struct BTHCIError *error );
	int32_t APICALL (*serialfile_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*serialcapture_ReceiveController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTRFCOMMCapture *capture, struct BTHCIError *error );
	enum BTHCIResult APICALL (*serialcapture_Receive)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMCapture *capture, struct BTHCIError *error );
	int32_t APICALL (*serialcapture_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*objectpush_PushController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTObjectPush *push, struct BTHCIError *error );
	enum BTHCIResult APICALL (*objectpush_Push)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTObjectPush *push, struct BTHCIError *error );
	int32_t APICALL (*objectpush_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*le_RunController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTLERequest *request, struct BTHCIError *error );
	enum BTHCIResult APICALL (*le_Run)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTLERequest *request, struct BTHCIError *error );
	int32_t APICALL (*le_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location, uint32_t acl );

	enum BTHCIResult APICALL (*a2dp_PlayController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTAudioPlayback *audio, struct BTHCIError *error );
	enum BTHCIResult APICALL (*a2dp_Play)( struct Bluetooth2IFace *Self, const struct BTACLTransport *transport,
		struct BTAudioPlayback *audio, struct BTHCIError *error );
	int32_t APICALL (*a2dp_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*pcm_PlayController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTAudioPCM *pcm, struct BTHCIError *error );
	enum BTHCIResult APICALL (*pcm_Play)( struct Bluetooth2IFace *Self, const struct BTACLTransport *transport,
		struct BTAudioPCM *pcm, struct BTHCIError *error );
	int32_t APICALL (*pcm_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*hid_MonitorController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTHIDMonitor *monitor, struct BTHCIError *error );
	enum BTHCIResult APICALL (*hid_Monitor)( struct Bluetooth2IFace *Self, const struct BTACLTransport *transport,
		struct BTHIDMonitor *monitor, struct BTHCIError *error );
	int32_t APICALL (*hid_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	enum BTHCIResult APICALL (*hidinput_ConnectController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTHIDInput *input, struct BTHCIError *error );
	enum BTHCIResult APICALL (*hidinput_Connect)( struct Bluetooth2IFace *Self, const struct BTACLTransport *transport,
		struct BTHIDInput *input, struct BTHCIError *error );
	int32_t APICALL (*hidinput_AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );

	uint32_t APICALL (*prefs_Adapters)( struct Bluetooth2IFace *Self, struct BT2PrefsAdapter *entries, uint32_t capacity );
	enum BTHCIResult APICALL (*prefs_Peers)( struct Bluetooth2IFace *Self, const uint8_t *local,
		struct BT2PrefsPeer *entries, uint32_t capacity, uint32_t *count );
	enum BTHCIResult APICALL (*prefs_Action)( struct Bluetooth2IFace *Self, const uint8_t *local,
		const uint8_t *peer, enum BT2PrefsAction action );
	// Supply either an inquiry or a bond request. The operation is synchronous.
	enum BTHCIResult APICALL (*prefs_PeerRequest)( struct Bluetooth2IFace *Self, const uint8_t *local,
		struct BTInquiryResult *inquiry, struct BTBondRequest *bond, struct BTHCIError *error );

	enum BTHCIResult APICALL (*fileshare_Listen)( struct Bluetooth2IFace *Self,
		struct BTFileShare *share, struct BTHCIError *error );
	enum BTHCIResult APICALL (*fileshare_Serve)( struct Bluetooth2IFace *Self,
		const struct BTACLTransport *transport, struct BTFileShare *share, struct BTHCIError *error );
	int32_t APICALL (*fileshare_AddController)( struct Bluetooth2IFace *Self,
		struct MsgPort *port, const struct BTControllerLocation *location );
	int32_t APICALL (*fileshare_Rule)( struct Bluetooth2IFace *Self, const uint8_t *local,
		const uint8_t *peer, int32_t access );

	enum BTHCIResult APICALL (*sharecontrol_Start)( struct Bluetooth2IFace *Self, const uint8_t *local, const char *path );
	void APICALL (*sharecontrol_Stop)( struct Bluetooth2IFace *Self );
	void APICALL (*sharecontrol_Status)( struct Bluetooth2IFace *Self, struct BTShareStatus *status );

	uint32_t APICALL (*control_Adapters)( struct Bluetooth2IFace *Self, struct BT2ManagedAdapter *entries, uint32_t capacity );
	enum BTHCIResult APICALL (*control_Submit)( struct Bluetooth2IFace *Self, enum BT2ManagedOperation operation,
		uint32_t registration, const struct BTPairRequest *pair, uint32_t *id );
	enum BTHCIResult APICALL (*control_Operation)( struct Bluetooth2IFace *Self, uint32_t id,
		struct BT2OperationStatus *status, struct BTInquiryDevice *devices, uint32_t capacity );
	enum BTHCIResult APICALL (*control_Cancel)( struct Bluetooth2IFace *Self, uint32_t id );
	enum BTHCIResult APICALL (*control_StartupShare)( struct Bluetooth2IFace *Self, struct BT2StartupShare *settings,
		uint32_t save );
	enum BTHCIResult APICALL (*control_AudioOutput)( struct Bluetooth2IFace *Self, struct BT2AudioOutput *settings,
		uint32_t save );
	enum BTHCIResult APICALL (*control_PlayWAV)( struct Bluetooth2IFace *Self, const char *path, uint32_t *id );
	uint32_t APICALL (*control_Playback)( struct Bluetooth2IFace *Self );
	enum BTHCIResult APICALL (*control_PeerAction)( struct Bluetooth2IFace *Self, const uint8_t *local,
		const uint8_t *peer, enum BT2PrefsAction action );
	enum BTHCIResult APICALL (*control_StartShare)( struct Bluetooth2IFace *Self, const uint8_t *local, const char *path );
	enum BTHCIResult APICALL (*control_StopShare)( struct Bluetooth2IFace *Self );
	enum BT2StartupShareState APICALL (*control_ShareStartupState)( struct Bluetooth2IFace *Self );
	int32_t APICALL (*control_ShareRule)( struct Bluetooth2IFace *Self, const uint8_t *local,
		const uint8_t *peer, int32_t access );
};

#endif
