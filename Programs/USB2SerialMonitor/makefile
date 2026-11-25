TARGET := USB2SerialMonitor
OBJDIR := obj
TOOL_BINDIR := bin

ifeq ($(shell uname),AmigaOS)
CC := gcc
else
CC := ppc-amigaos-gcc
endif

CFLAGS := -O2 -Wall -Wextra -MMD -MP -mcrt=newlib -I../../inc_Public -D__USE_INLINE__
SOURCES := $(sort $(wildcard src/*.c))
OBJECTS := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(TOOL_BINDIR)/$(TARGET)

$(TOOL_BINDIR)/$(TARGET): $(OBJECTS)
	mkdir -p $(TOOL_BINDIR)
	$(CC) -mcrt=newlib -o $@ $(OBJECTS)

$(OBJDIR)/%.o: src/%.c makefile
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(TOOL_BINDIR)/$(TARGET)

-include $(OBJECTS:.o=.d)
