#
# Video external USB2 class driver for AmigaOS 4
#

.PHONY: all clean install

TARGET := cls_Video.Driver
OBJDIR := obj
BINDIR := bin
HOSTOS := $(shell uname)

CFLAGS := -O2 -I. -I../../inc_Public -W -Wall -MMD -MP -D__USE_INLINE__ -fno-builtin
LDFLAGS := -nostartfiles

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

SRCS := $(sort $(wildcard src/*.c))
OBJS := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BINDIR):
	$(MKDIR) $(BINDIR)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

-include $(DEPS)

clean:
	$(RM) $(OBJDIR) $(BINDIR)

install: all
ifeq ($(HOSTOS),AmigaOS)
	copy $(BINDIR)/$(TARGET) Libs:USB2
	copy cls_Video.Class Libs:USB2
	copy vid_PlayStationEye.Class Libs:USB2
else
	@echo "Copy $(BINDIR)/$(TARGET), cls_Video.Class and vid_PlayStationEye.Class to Libs:USB2 on AmigaOS."
endif
