CFLAGS:=-MMD -g -Wall -Wextra
LDFLAGS:=-g
STRIP:=strip

TARGET:=mvdtool

SRCFILES:=$(wildcard *.c)
OBJFILES:=$(SRCFILES:%.c=%.o)

.PHONY: all default clean

all: $(TARGET)

default: $(TARGET)

-include *.d

$(TARGET): $(OBJFILES) 
	$(CC) -o $@ $(LDFLAGS) $^

strip: $(TARGET)
	$(STRIP) $^

clean:
	rm -f $(TARGET) *.o *.d
