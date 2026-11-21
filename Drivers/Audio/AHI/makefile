TARGET := usb2.audio
BINDIR := bin
MODE_ID := 0x7f000001
BT_MODE_ID := 0x7f000002
BT_PARSE := ../../../Programs/USB2BluetoothTest/src/ParseAddress.c
HOSTOS := $(shell uname)
CFLAGS := -O2 -Wall -Wextra -D__USE_INLINE__ -I../../../inc_Public

ifeq ($(HOSTOS),AmigaOS)
CC := gcc
OBJCOPY := objcopy
MKDIR := makedir ALL
RM := delete
else ifeq ($(HOSTOS),Linux)
CC := ppc-amigaos-gcc
OBJCOPY := ppc-amigaos-objcopy
MKDIR := mkdir -p
RM := rm -f
else
$(error Unknown host: $(HOSTOS))
endif

.PHONY: all clean force_mode

all: $(BINDIR)/$(TARGET) $(BINDIR)/USB2 $(BINDIR)/bluetooth2.audio $(BINDIR)/Bluetooth2

$(BINDIR)/$(TARGET): USB2AHI.c ../inc/AudioDevice.h | $(BINDIR)
	$(CC) $(CFLAGS) -nostartfiles -o $@ USB2AHI.c

$(BINDIR)/bluetooth2.audio: USB2AHI.c BluetoothPlayback.c ../inc/AudioDevice.h $(BT_PARSE) \
		../../../inc_Public/Bluetooth.h ../../../inc_Public/interfaces/bluetooth2.h | $(BINDIR)
	$(CC) $(CFLAGS) -DBLUETOOTH_AHI -nostartfiles -o $@ USB2AHI.c $(BT_PARSE)

$(BINDIR)/USB2: $(BINDIR)/USB2Mode-$(MODE_ID).o force_mode
	$(OBJCOPY) -O binary -j .rodata $< $@

$(BINDIR)/Bluetooth2: $(BINDIR)/Bluetooth2Mode-$(BT_MODE_ID).o force_mode
	$(OBJCOPY) -O binary -j .rodata $< $@

$(BINDIR)/USB2Mode-$(MODE_ID).o: USB2Mode.S | $(BINDIR)
	$(CC) -DUSB2_AHI_MODE_ID=$(MODE_ID) -c $< -o $@

$(BINDIR)/Bluetooth2Mode-$(BT_MODE_ID).o: USB2Mode.S | $(BINDIR)
	$(CC) -DBLUETOOTH_AHI -DUSB2_AHI_MODE_ID=$(BT_MODE_ID) -c $< -o $@

$(BINDIR):
	$(MKDIR) $(BINDIR)

clean:
	$(RM) $(BINDIR)/$(TARGET) $(BINDIR)/USB2 $(BINDIR)/USB2Mode-*.o
	$(RM) $(BINDIR)/bluetooth2.audio $(BINDIR)/Bluetooth2 $(BINDIR)/Bluetooth2Mode-*.o
