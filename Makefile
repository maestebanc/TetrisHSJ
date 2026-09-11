CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
LDFLAGS = -lSDL2 -lm

ZIG = $(HOME)/.local/share/mise/installs/zig/0.13.0/bin/zig
WIN_INCLUDES = -Isrc -Iwin_sdk/include
WIN_LIBS = -Lwin_sdk/lib -lmingw32 -lSDL2main win_sdk/lib/libSDL2.dll.a -lm
WIN_FLAGS = -target x86_64-windows -Wall -Wextra -O2 -Wl,/subsystem:windows

SRC = src/main.c src/game.c src/render.c src/audio.c src/font.c
HDR = src/game.h src/render.h src/audio.h src/font.h

all: windows linux

windows: SantJoanTetris.exe

SantJoanTetris.exe: $(SRC) $(HDR)
	$(ZIG) cc $(WIN_FLAGS) $(WIN_INCLUDES) -o SantJoanTetris.exe $(SRC) $(WIN_LIBS)

linux: sant_joan_tetris

sant_joan_tetris: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o sant_joan_tetris $(SRC) $(LDFLAGS)

package-windows: SantJoanTetris.exe
	zip -r SantJoanTetris_Windows.zip SantJoanTetris.exe SDL2.dll LEEME_WINDOWS.txt highscore.txt assets/

clean:
	rm -f sant_joan_tetris SantJoanTetris.exe SantJoanTetris_Windows.zip

.PHONY: all windows linux package-windows clean
