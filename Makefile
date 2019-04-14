CFLAGS := -MMD -g -O2 -Wall -Wextra
LDFLAGS := -g
STRIP := strip

TARGET := mvdtool

SRCFILES := $(wildcard *.c)
OBJFILES := $(SRCFILES:%.c=%.o)

all: $(TARGET)

default: $(TARGET)

.PHONY: all default clean

-include *.d

$(TARGET): $(OBJFILES)
	$(CC) -o $@ $(LDFLAGS) $^

strip: $(TARGET)
	$(STRIP) $^

clean:
	rm -f $(TARGET) *.o *.d
