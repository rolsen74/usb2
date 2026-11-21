TARGET := Bluetooth.Driver
OBJDIR := obj
BINDIR := bin

ifeq ($(shell uname),AmigaOS)
CC := gcc
else
CC := ppc-amigaos-gcc
endif

CFLAGS := -O2 -Wall -Wextra -ffreestanding -MMD -MP -D__USE_INLINE__ -I../../../inc_Public
LDFLAGS := -nostdlib -Wl,-e,_start
SOURCES := $(sort $(wildcard src/*.c))
OBJECTS := $(patsubst src/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) makefile
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(OBJDIR)/%.o: src/%.c makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(BINDIR)/$(TARGET)

-include $(OBJECTS:.o=.d)
