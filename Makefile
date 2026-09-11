CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
LDFLAGS = -lSDL2 -lm

ZIG = $(HOME)/.local/share/mise/installs/zig/0.13.0/bin/zig
WIN_INCLUDES = -Isrc -Iwin_sdk/include
WIN_LIBS = -Lwin_sdk/lib -lmingw32 -lSDL2main win_sdk/lib/libSDL2.dll.a -lm
WIN_FLAGS = -target x86_64-windows -Wall -Wextra -O2 -Wl,/subsystem:windows

WINDRES = llvm-windres

SRC = src/main.c src/game.c src/render.c src/audio.c src/font.c
HDR = src/game.h src/render.h src/audio.h src/font.h

MAC_INCLUDES = -Isrc -Imac_sdk/include
MAC_LIBS = -Lmac_sdk/lib -lSDL2 -lm -Wl,-rpath,@executable_path/ -Wl,-rpath,@executable_path/../Frameworks
MAC_FLAGS = -target aarch64-macos -Wall -Wextra -O2

all: windows linux mac

windows: SantJoanTetris.exe

manifest.res.o: manifest.rc SantJoanTetris.exe.manifest
	$(WINDRES) manifest.rc -O coff -o manifest.res.o

SantJoanTetris.exe: $(SRC) $(HDR) manifest.res.o
	$(ZIG) cc $(WIN_FLAGS) $(WIN_INCLUDES) -o SantJoanTetris.exe $(SRC) manifest.res.o $(WIN_LIBS)

linux: sant_joan_tetris

sant_joan_tetris: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o sant_joan_tetris $(SRC) $(LDFLAGS)

mac: sant_joan_tetris_mac

sant_joan_tetris_mac: $(SRC) $(HDR)
	$(ZIG) cc $(MAC_FLAGS) $(MAC_INCLUDES) -o sant_joan_tetris_mac $(SRC) $(MAC_LIBS)

mac-app: sant_joan_tetris_mac
	rm -rf SantJoanTetris.app
	mkdir -p SantJoanTetris.app/Contents/MacOS
	mkdir -p SantJoanTetris.app/Contents/Resources/assets
	mkdir -p SantJoanTetris.app/Contents/Frameworks
	cp mac_app/Info.plist SantJoanTetris.app/Contents/
	cp assets/AppIcon.icns SantJoanTetris.app/Contents/Resources/
	cp assets/* SantJoanTetris.app/Contents/Resources/assets/
	mkdir -p SantJoanTetris.app/Contents/MacOS/assets
	cp assets/* SantJoanTetris.app/Contents/MacOS/assets/
	cp sant_joan_tetris_mac SantJoanTetris.app/Contents/MacOS/SantJoanTetris
	chmod +x SantJoanTetris.app/Contents/MacOS/SantJoanTetris
	cp mac_sdk/lib/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/Frameworks/
	cp mac_sdk/lib/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/MacOS/

package-windows: SantJoanTetris.exe
	zip -r SantJoanTetris_Windows.zip SantJoanTetris.exe SantJoanTetris.exe.manifest SDL2.dll LEEME_WINDOWS.txt highscore.txt assets/

package-mac: mac-app
	rm -f SantJoanTetris_macOS_Silicon.zip
	zip -r SantJoanTetris_macOS_Silicon.zip SantJoanTetris.app sant_joan_tetris_mac run_mac.sh LEEME_MAC.txt assets/ mac_sdk/lib/libSDL2-2.0.0.dylib

clean:
	rm -rf sant_joan_tetris SantJoanTetris.exe SantJoanTetris_Windows.zip manifest.res.o sant_joan_tetris_mac SantJoanTetris.app SantJoanTetris_macOS_Silicon.zip

.PHONY: all windows linux mac mac-app package-windows package-mac clean
