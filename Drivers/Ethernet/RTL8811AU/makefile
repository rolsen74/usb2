
#
#      -+- Universal serial bus -+-
# Copyright (c) 2012-2026 by Rene W. Olsen
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

TARGET := eth_RTL8811AU.Driver
OBJDIR := obj
BINDIR := bin

ifeq ($(shell uname),AmigaOS)
CC := gcc
MKDIR := makedir FORCE
else
CC := ppc-amigaos-gcc
MKDIR := mkdir -p
endif

CFLAGS := -O2 -Wall -Wextra -mcrt=newlib -MMD -MP -D__USE_INLINE__ -I../../../inc_Public -I../Common -Isrc
LDFLAGS := -nostartfiles -mcrt=newlib -Wl,-e,_start
SOURCES := $(sort $(wildcard src/*.c))
OBJECTS := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SOURCES))

OBJECTS += $(patsubst ../Common/%.c,$(OBJDIR)/common/%.o,$(wildcard ../Common/EthClass_*.c ../Common/Sana2_*.c))
OBJECTS += $(addprefix $(OBJDIR)/common/,Wireless_AESBlock.o Wireless_AESExpand.o Wireless_AESSubstitute.o Wireless_CCMP.o)
OBJECTS += $(addprefix $(OBJDIR)/common/,Wireless_StartAuto.o Wireless_StartPassword.o)

.PHONY: all clean

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) makefile
	$(MKDIR) $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(OBJDIR)/%.o: src/%.c makefile
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/common/%.o: ../Common/%.c makefile
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(BINDIR)/$(TARGET)

-include $(OBJECTS:.o=.d)
