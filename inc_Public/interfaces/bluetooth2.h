#ifndef INTERFACES_BLUETOOTH2_H
#define INTERFACES_BLUETOOTH2_H

#include <exec/interfaces.h>
#include <exec/ports.h>
#include "Bluetooth.h"

#define BLUETOOTH2_NAME "bluetooth2.device"
#define BLUETOOTH2_INTERFACE_VERSION 3

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
};

#define BLUETOOTH2_PAIRING_INTERFACE "pairing"
#define BLUETOOTH2_PAIRING_VERSION 1

struct Bluetooth2PairingIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2PairingIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2PairingIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2PairingIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2PairingIFace *Self );

	enum BTHCIResult APICALL (*PairController)( struct Bluetooth2PairingIFace *Self, uint32_t unit,
		const struct BTPairRequest *pair, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Pair)( struct Bluetooth2PairingIFace *Self,
		const struct BTHCITransport *transport, const struct BTPairRequest *pair, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2PairingIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_BONDING_INTERFACE "bonding"
#define BLUETOOTH2_BONDING_VERSION 1

struct Bluetooth2BondingIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2BondingIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2BondingIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2BondingIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2BondingIFace *Self );

	enum BTHCIResult APICALL (*BondController)( struct Bluetooth2BondingIFace *Self, uint32_t unit,
		struct BTBondRequest *bond, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Bond)( struct Bluetooth2BondingIFace *Self,
		const struct BTHCITransport *transport, struct BTBondRequest *bond, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2BondingIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#endif
