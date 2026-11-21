# USB CCID Smart Cards

USB2 binds standard bulk CCID reader interfaces with class `0B`, subclass
`00`, and protocol `00`. A reader receives the lowest available unit
number, starting at 0. Each slot can be queried independently.

Copy `SmartCard.Class` and `SmartCard.Driver` to `LIBS:USB2/` and reconnect
the reader. Run these commands from a Shell.

```text
USB2SmartCardTest
USB2SmartCardTest UNIT=0 SLOT=0
USB2SmartCardTest UNIT=0 SLOT=0 ATR
```

The first command lists readers. The second checks whether a card is
present in slot 0. `ATR` powers on the card, prints its answer to reset
and leaves it active. Power-on requires a reader with automatic voltage
selection.

AmigaOS 4 applications can open the active `SmartCard.Driver` library
and obtain its `ccid` version 1 interface. The declarations are in
`inc_Public/interfaces/usb2smartcard.h`. Use `GetInfo` to enumerate
readers and `Command` to check a slot, power a card on or off, or send a
short APDU to a reader that supports APDU-level exchange. Exchange
responses include the card's two status bytes. Command replies also
provide the slot state and CCID reader error. Provide an output buffer
of at least `USB2SMARTCARD_ATR_MAX` for power-on or
`USB2SMARTCARD_APDU_RESPONSE_MAX` for exchange.

The driver is experimental. Card state is queried on demand.
PC/SC integration, interrupt notifications, PIN-pad commands, TPDU or
character-level exchanges, extended APDUs, and USB ICCD use separate
paths and are outside this implementation. No reader has been tested
on Amiga hardware.
