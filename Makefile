CFLAGS := -MMD -g -O2 -Wall -Wextra
LDFLAGS := -g
STRIP := strip

TARGET := mvdtool

SRCFILES := $(wildcard src/*.c)
OBJFILES := $(SRCFILES:%.c=%.o)

all: $(TARGET)

default: $(TARGET)

.PHONY: all default clean

-include src/*.d

$(TARGET): $(OBJFILES)
	$(CC) -o $@ $(LDFLAGS) $^

strip: $(TARGET)
	$(STRIP) $^

clean:
	rm -f $(TARGET) src/*.o src/*.d
