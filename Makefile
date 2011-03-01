.PHONY: all default win32-exe clean

all: default

default:
	$(MAKE) -C src default

win32-exe:
	$(MAKE) -C src CC=i586-mingw32msvc-gcc STRIP=i586-mingw32msvc-strip TARGET=mvdtool.exe strip

clean:
	$(MAKE) -C src clean
	$(MAKE) -C src TARGET=mvdtool.exe clean
