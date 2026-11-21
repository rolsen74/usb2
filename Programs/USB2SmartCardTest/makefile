#
# USB2 CCID smart-card test for AmigaOS 4
#

.PHONY: all clean

TARGET := USB2SmartCardTest
OBJDIR := obj
BINDIR := bin
HOSTOS := $(shell uname)

CFLAGS := -O2 -I. -I../../inc_Public -W -Wall -MMD -MP -mcrt=newlib
LDFLAGS := -mcrt=newlib

ifeq ($(HOSTOS),AmigaOS)
CC := gcc
MKDIR := makedir ALL
RM := delete
else ifeq ($(HOSTOS),Linux)
CC := ppc-amigaos-gcc
MKDIR := mkdir -p
RM := rm -rf
else
$(error Unknown host: $(HOSTOS))
endif

SRCS := $(sort $(wildcard *.c))
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BINDIR):
	$(MKDIR) $(BINDIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

-include $(DEPS)

clean:
	$(RM) $(OBJDIR) $(BINDIR)
