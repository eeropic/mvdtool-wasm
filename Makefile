MODE := native

SRCFILES := $(wildcard src/*.c)
OBJFILES := $(SRCFILES:%.c=%.o)

NATIVE_CC := gcc
NATIVE_CFLAGS := -MMD -g -O2 -Wall -Wextra
NATIVE_LDFLAGS := -g
NATIVE_STRIP := strip
NATIVE_TARGET := mvdtool

WASM_CC := emcc
WASM_CFLAGS := -MMD -gsource-map -O2 -Wall -Wextra
WASM_LDFLAGS := -g -s FORCE_FILESYSTEM=1 -s EXPORTED_FUNCTIONS='["_main", "_malloc", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "allocate", "stringToUTF8", "UTF8ToString", "lengthBytesUTF8", "setValue", "getValue"]' -s ASSERTIONS=2 -gsource-map -s SAFE_HEAP=1 -s INITIAL_MEMORY=64MB -s MAXIMUM_MEMORY=256MB -s STACK_SIZE=64KB
WASM_TARGET := output-web/mvdtool.html
WASM_SHELL := --shell-file src-web/shell_minimal.html

all: $(MODE)

.PHONY: all native wasm clean strip

native: MODE := native
native: CC := $(NATIVE_CC)
native: CFLAGS := $(NATIVE_CFLAGS)
native: LDFLAGS := $(NATIVE_LDFLAGS)
native: STRIP := $(NATIVE_STRIP)
native: TARGET := $(NATIVE_TARGET)
native: $(NATIVE_TARGET)

wasm: MODE := wasm
wasm: CC := $(WASM_CC)
wasm: CFLAGS := $(WASM_CFLAGS)
wasm: LDFLAGS := $(WASM_LDFLAGS)
wasm: TARGET := $(WASM_TARGET)
wasm: $(WASM_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(NATIVE_TARGET): $(OBJFILES)
	$(CC) -o $@ $(LDFLAGS) $^

$(WASM_TARGET): $(OBJFILES)
	mkdir -p output-web
	$(CC) $(OBJFILES) $(LDFLAGS) -o $@ $(WASM_SHELL)

strip: $(NATIVE_TARGET)
	$(STRIP) $^

clean:
	rm -f $(NATIVE_TARGET) $(WASM_TARGET) src/*.o src/*.d
	rm -rf output-web