TARGET := USB2WaitEthernet
BINDIR := bin

ifeq ($(shell uname),AmigaOS)
CC := gcc
MKDIR := makedir ALL
RM := delete
else
CC := ppc-amigaos-gcc
MKDIR := mkdir -p
RM := rm -rf
endif

CFLAGS := -O2 -Wall -Wextra -mcrt=newlib -I../../inc_Public

.PHONY: all clean

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): USB2WaitEthernet.c ../../inc_Public/interfaces/usb2eth.h makefile
	$(MKDIR) $(BINDIR)
	$(CC) $(CFLAGS) -o $@ USB2WaitEthernet.c

clean:
	$(RM) $(BINDIR)
