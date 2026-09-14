CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
LDFLAGS = -lSDL2 -lm

ZIG ?= zig
WIN_INCLUDES = -Isrc -Iwin_sdk/include
WIN_LIBS = -Lwin_sdk/lib -lmingw32 -lSDL2main win_sdk/lib/libSDL2.dll.a -lm
WIN_FLAGS = -target x86_64-windows -Wall -Wextra -O2 -Wl,/subsystem:windows

WINDRES = llvm-windres

SRC = src/main.c src/game.c src/render.c src/audio.c src/font.c
HDR = src/game.h src/render.h src/audio.h src/font.h

# El target "mac" tiene dos caminos:
#  - Compilando EN un Mac real (p.ej. el runner macos-latest de CI): usa el
#    clang y el SDL2 nativos del sistema (instalado con `brew install sdl2`).
#    Es lo más fiable, y es lo que de verdad se puede firmar y ejecutar.
#  - Compilando desde Linux (este equipo de desarrollo): usa zig cc en modo
#    cross-compile contra el mac_sdk/ vendorizado. Sirve para detectar errores
#    de compilación pronto, pero el binario resultante NO se puede firmar ni
#    probar aquí — para eso está el workflow de CI en macos-latest.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	MAC_CC = cc
	# OJO: `sdl2-config --cflags` en Homebrew da -I<prefix>/include/SDL2 (pensado
	# para #include <SDL.h> a secas), pero el código de aquí usa #include
	# <SDL2/SDL.h> en todas las plataformas (como en Linux/Windows). Por eso se
	# usa el prefijo de Homebrew directamente y se apunta a .../include (el
	# padre de SDL2/), no al que da sdl2-config, o el compilador no encuentra
	# 'SDL2/SDL.h' (busca .../include/SDL2/SDL2/SDL.h, duplicado).
	MAC_SDL_PREFIX = $(shell brew --prefix sdl2 2>/dev/null)
	MAC_INCLUDES = -Isrc -I$(MAC_SDL_PREFIX)/include
	MAC_LIBS = $(shell sdl2-config --libs) -lm
	MAC_FLAGS = -Wall -Wextra -O2
else
	MAC_CC = $(ZIG) cc
	MAC_INCLUDES = -Isrc -Imac_sdk/include
	MAC_LIBS = -Lmac_sdk/lib -lSDL2 -lm -Wl,-rpath,@executable_path/ -Wl,-rpath,@executable_path/../Frameworks
	MAC_FLAGS = -target aarch64-macos -Wall -Wextra -O2
endif

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
	$(MAC_CC) $(MAC_FLAGS) $(MAC_INCLUDES) -o sant_joan_tetris_mac $(SRC) $(MAC_LIBS)

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
ifeq ($(UNAME_S),Darwin)
	@# Compilado en un Mac real: el binario quedó enlazado contra el SDL2 de
	@# Homebrew por una ruta absoluta (p.ej. /opt/homebrew/...), que no existe
	@# en el Mac de quien lo descargue. Hay que copiar ESE dylib concreto y
	@# reescribir la referencia a una ruta relativa (@executable_path) para
	@# que el .app sea de verdad autónomo, en vez de copiar a ciegas el dylib
	@# vendorizado (que ni siquiera es el que se usó para enlazar).
	@SDL_DYLIB=$$(otool -L SantJoanTetris.app/Contents/MacOS/SantJoanTetris | awk '/libSDL2.*\.dylib/{print $$1; exit}'); \
	echo "Empaquetando $$SDL_DYLIB junto al ejecutable..."; \
	cp "$$SDL_DYLIB" SantJoanTetris.app/Contents/MacOS/libSDL2-2.0.0.dylib; \
	install_name_tool -id @executable_path/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/MacOS/libSDL2-2.0.0.dylib; \
	install_name_tool -change "$$SDL_DYLIB" @executable_path/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/MacOS/SantJoanTetris; \
	cp SantJoanTetris.app/Contents/MacOS/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/Frameworks/
	@# CRÍTICO en Apple Silicon: macOS se niega a ejecutar binarios arm64 sin
	@# firmar (exigencia del kernel, no solo de Gatekeeper). Firma ad-hoc, sin
	@# necesitar cuenta de pago de Apple: basta para que arranque en cualquier Mac.
	@echo "Firmando SantJoanTetris.app (ad-hoc)..."
	@codesign --force --deep --sign - SantJoanTetris.app
	@echo "OK: firmado. Verificando..."
	@codesign --verify --deep --strict SantJoanTetris.app && echo "Firma verificada correctamente."
else
	cp mac_sdk/lib/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/Frameworks/
	cp mac_sdk/lib/libSDL2-2.0.0.dylib SantJoanTetris.app/Contents/MacOS/
	@echo "AVISO: compilado en Linux (cross), no en un Mac real."
	@echo "       'codesign' no existe fuera de macOS, así que este .app NO"
	@echo "       arrancará en Apple Silicon tal cual. Es solo para detectar"
	@echo "       errores de compilación pronto — el .app que de verdad se"
	@echo "       distribuye lo firma el workflow de CI en macos-latest"
	@echo "       (ver .github/workflows/build.yml), o hazlo tú a mano en un"
	@echo "       Mac con: codesign --force --deep --sign - SantJoanTetris.app"
endif

package-windows: SantJoanTetris.exe
	rm -f SantJoanTetris_Windows.zip
	zip -j SantJoanTetris_Windows.zip SantJoanTetris.exe SantJoanTetris.exe.manifest win_sdk/bin/SDL2.dll LEEME_WINDOWS.txt highscore.txt
	zip -r SantJoanTetris_Windows.zip assets/

package-mac: mac-app
	rm -f SantJoanTetris_macOS_Silicon.zip
	zip -r SantJoanTetris_macOS_Silicon.zip SantJoanTetris.app sant_joan_tetris_mac run_mac.sh LEEME_MAC.txt assets/
	@# El dylib vendorizado suelto solo hace falta si el .app se cruzó desde
	@# Linux (sant_joan_tetris_mac / run_mac.sh sueltos lo necesitan al lado).
	@# En un Mac real el .app ya lleva su propio SDL2 autónomo dentro.
ifneq ($(UNAME_S),Darwin)
	zip SantJoanTetris_macOS_Silicon.zip mac_sdk/lib/libSDL2-2.0.0.dylib
endif

package-web: SantJoanTetris.html
	rm -f SantJoanTetris_Web.zip
	zip -j SantJoanTetris_Web.zip SantJoanTetris.html LEEME_WEB.txt

package-linux: sant_joan_tetris
	rm -f SantJoanTetris_Linux.tar.gz
	tar -czf SantJoanTetris_Linux.tar.gz sant_joan_tetris run.sh LEEME_LINUX.txt assets/

PKG_VERSION = 1.0.4
LINUX_STAGE = linux_pkg_stage

# Árbol de instalación FHS (usr/bin, usr/share/...) compartido por los
# targets package-deb y package-rpm, para no duplicar rutas entre ambos.
# El único archivo que el binario carga en tiempo de ejecución desde disco
# es el icono de ventana (assets/icon.bmp); todo lo demás (fuente, audio)
# está embebido en el propio ejecutable, así que instalar bien esa única
# ruta (ver los candidatos añadidos en main.c) es lo único que hace falta.
stage-linux-files: linux
	rm -rf $(LINUX_STAGE)
	mkdir -p $(LINUX_STAGE)/usr/bin
	mkdir -p $(LINUX_STAGE)/usr/share/applications
	mkdir -p $(LINUX_STAGE)/usr/share/pixmaps
	mkdir -p $(LINUX_STAGE)/usr/share/icons/hicolor/256x256/apps
	mkdir -p $(LINUX_STAGE)/usr/share/icons/hicolor/scalable/apps
	mkdir -p $(LINUX_STAGE)/usr/share/sant-joan-tetris/assets
	mkdir -p $(LINUX_STAGE)/usr/share/doc/santjoantetris
	cp sant_joan_tetris $(LINUX_STAGE)/usr/bin/
	cp sant_joan_tetris.desktop $(LINUX_STAGE)/usr/share/applications/
	cp assets/ma_tetris.png $(LINUX_STAGE)/usr/share/pixmaps/
	cp assets/ma_tetris.png $(LINUX_STAGE)/usr/share/icons/hicolor/256x256/apps/
	cp assets/ma_tetris.svg $(LINUX_STAGE)/usr/share/icons/hicolor/scalable/apps/
	cp assets/icon.bmp $(LINUX_STAGE)/usr/share/sant-joan-tetris/assets/
	cp LICENSE $(LINUX_STAGE)/usr/share/doc/santjoantetris/

# .deb y .rpm se generan con fpm (https://github.com/jordansissel/fpm), que
# construye ambos formatos desde el mismo árbol ya en rutas FHS, sin tener
# que mantener a mano un debian/control y un .spec de rpm por separado.
# Requiere el gem "fpm" instalado (y dpkg-deb / rpmbuild según el formato);
# no forma parte de "package-all" porque este equipo de desarrollo (Arch)
# no lleva esas herramientas — se ejecuta en el job de Linux de la CI
# (ubuntu-latest), que sí las tiene.
package-deb: stage-linux-files
	rm -f santjoantetris_$(PKG_VERSION)_amd64.deb
	fpm -s dir -t deb \
	  -n santjoantetris -v $(PKG_VERSION) \
	  --license MIT \
	  --category games \
	  --url "https://github.com/maestebanc/TetrisHSJ" \
	  --maintainer "Miguel Angel Esteban <maestebanc@gmail.com>" \
	  --description "Tetris tematizado del Hospital Universitario de Sant Joan d'Alacant, ambientado en el equipo de Informaticos de Guardia." \
	  --depends libsdl2-2.0-0 \
	  -C $(LINUX_STAGE) -p santjoantetris_$(PKG_VERSION)_amd64.deb \
	  usr

package-rpm: stage-linux-files
	rm -f santjoantetris-$(PKG_VERSION)-1.x86_64.rpm
	fpm -s dir -t rpm \
	  -n santjoantetris -v $(PKG_VERSION) \
	  --license MIT \
	  --category games \
	  --url "https://github.com/maestebanc/TetrisHSJ" \
	  --maintainer "Miguel Angel Esteban <maestebanc@gmail.com>" \
	  --description "Tetris tematizado del Hospital Universitario de Sant Joan d'Alacant, ambientado en el equipo de Informaticos de Guardia." \
	  --depends SDL2 \
	  -C $(LINUX_STAGE) -p santjoantetris-$(PKG_VERSION)-1.x86_64.rpm \
	  usr

package-all: package-linux package-windows package-mac package-web

clean:
	rm -rf sant_joan_tetris SantJoanTetris.exe SantJoanTetris_Windows.zip manifest.res.o sant_joan_tetris_mac SantJoanTetris.app SantJoanTetris_macOS_Silicon.zip SantJoanTetris_Web.zip SantJoanTetris_Linux.tar.gz $(LINUX_STAGE) *.deb *.rpm

.PHONY: all windows linux mac mac-app package-windows package-mac package-web package-linux stage-linux-files package-deb package-rpm package-all clean
