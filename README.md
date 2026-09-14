# 🏥 Sant Joan Tetris
### *Unidad de Informática y Tecnologías de la Información (TIC)*
### *Hospital Universitario de Sant Joan d'Alacant*

*(Idea original y prompt: Miguel Ángel Esteban - aka MA - 2026)*
*(Desarrollo y mejoras de la versión 1.0.4 asistidos con Claude, Anthropic - 2026)*

Un juego de Tetris ambientado en el día a día (y la noche a noche) del equipo de **Informáticos de Guardia** del Hospital Universitario de Sant Joan d'Alacant (**Jose María, Marichu, Diego y Ernesto**). Disponible para Linux (binario, `.deb`, `.rpm` y Flatpak), Windows, Mac, y directamente en el navegador (incluido móvil/tablet).

## Novedades de la versión 1.0.4

* **Nuevos paquetes de Linux**: además del `.tar.gz` portable, ahora hay `.deb` (Debian/Ubuntu), `.rpm` (Fedora/openSUSE) y un **Flatpak** instalable en cualquier distribución — los tres añaden el juego al menú de aplicaciones con su icono.
* Se añadió una licencia MIT al proyecto.

## Novedades de la versión 1.0.3

* **Arreglado el sonido en iPhone/Safari (versión Web):** Safari en iOS es especialmente estricto desbloqueando el audio — no basta con reanudar el `AudioContext`, hace falta además reproducir un sonido real (aunque sea silencioso) dentro del mismo toque del usuario. Se añade ese desbloqueo, más una red de seguridad en el primer toque de la página y una reanudación automática al volver de segundo plano (bloqueo de pantalla, cambio de app).

## Novedades de la versión 1.0.2

* **La versión Web ya funciona en móvil y tablet:** antes solo se podía jugar con teclado, así que en un dispositivo táctil el juego era inutilizable. Ahora incluye un panel de controles en pantalla (mover, rotar, caída rápida, hold, pausa) que aparece automáticamente en pantallas táctiles, y un `viewport` correcto para que no se vea diminuto.
* **Arreglada la pieza en T de la versión Web:** al girarla en un sentido perdía una celda y se veía rota; ya gira correctamente como en las versiones nativas.

## Novedades de la versión 1.0.1

* **Arreglado el arranque en Mac (Apple Silicon):** el motivo real era que macOS exige que todo binario arm64 esté **firmado** para poder ejecutarse — no es solo un aviso de Gatekeeper, es el propio kernel el que rechaza binarios sin firmar, y la 1.0.0 nunca firmaba el `.app`. Ahora `make package-mac` firma con firma ad-hoc (sin necesitar cuenta de pago de Apple), y el workflow de CI (`.github/workflows/build.yml`) compila de forma **nativa en un Mac real** (`macos-latest`) en cada push, para que el binario que se distribuye esté genuinamente construido y firmado en hardware Apple — no cruzado desde Linux.
* **Versión Web con el mismo aspecto y la misma música que la nativa:** misma fuente de píxeles, mismos paneles, mismos bloques y el motor de audio (bombo, bajo, pads, melodías) portado del código C — antes eran completamente distintos entre sí.
* **Menú principal navegable** con flechas / teclas numéricas + Intro, igual en las cuatro versiones.
* **Más incidencias y más variedad** en los mensajes de línea completada (single, doble, triple, Tetris), elegidos al azar en vez de ser siempre el mismo texto.
* **El informático que abre el turno ahora es aleatorio** cada partida, en vez de ser siempre Jose María.
* Limpieza de referencias técnicas (resoluciones, "Full HD"...) del texto del juego.
* Ese mismo workflow de CI compila y verifica las cuatro versiones (Linux, Windows, macOS y Web) en cada cambio.

![Partida en vivo - Sant Joan Tetris](docs/screenshots/gameplay.png)

---

## 📸 Galería de Capturas

| Menú Principal e Inicio | Turno de Guardia en Vivo |
|:---:|:---:|
| ![Pantalla de Inicio](docs/screenshots/inicio.png) | ![Partida en Vivo](docs/screenshots/gameplay.png) |
| **Manual de Guardia (F1)** | **Pausa de Guardia (Café en Urgencias)** |
| ![Manual de Guardia](docs/screenshots/manual_guardia.png) | ![Pausa de Guardia](docs/screenshots/pausa_cafe.png) |

---

## 🌟 La Realidad de la Guardia en Sant Joan

En el Hospital de Sant Joan, el equipo de guardia (**Jose María, Marichu, Diego y Ernesto**) vela por los sistemas 24 horas al día. La noche avanza tranquila... hasta que suena el teléfono de guardia a las 03:40 AM:
* La impresora de Urgencias se ha tragado 40 etiquetas de triaje.
* En Planta 3 juran que el ratón "ha muerto solo" (estaba desenchufado tras mover la papelera).
* Alguien en la UCI ha desenchufado el cable de red del switch para poner a cargar el móvil.
* Y en Consultas, el clásico: *"Doctor, ¿ha probado a apagarlo y volverlo a encender?"* obra el milagro una vez más.

En **Sant Joan Tetris**, cada línea que completas es un ticket absurdo que consigues resolver antes de que los médicos tengan que volver al papel y bolígrafo BIC.

---

## 🪟 Cómo Jugar en Windows (10 / 11)

1. **Ejecución directa:**
   * Haz doble clic en **`SantJoanTetris.exe`**.
   * No requiere instalación ni configuraciones (`SDL2.dll` ya está incluida en la carpeta).
2. **Paquete ZIP para llevar:**
   * Tienes el archivo **`SantJoanTetris_Windows.zip`** listo para enviar o copiar a cualquier PC.

---

## 🐧 Cómo Jugar en Linux

```bash
./sant_joan_tetris
# o bien:
./run.sh
```

---

## 🌐 Cómo Jugar en el Navegador (Web)

Funciona en cualquier sistema operativo, sin instalar nada:

1. Descomprime **`SantJoanTetris_Web.zip`** (o usa directamente el archivo `SantJoanTetris.html` del repositorio).
2. Haz doble clic en **`SantJoanTetris.html`** para abrirlo con Chrome, Edge, Firefox o Safari.

Es un único archivo autocontenido: mismo código C portado a JavaScript, mismo aspecto (fuente de píxeles, paneles, bloques) y el mismo motor de audio que la versión nativa. No necesita servidor ni conexión a internet.

---

## 🍏 Cómo Jugar en Mac (Apple Silicon: M1 / M2 / M3 / M4)

`SantJoanTetris_macOS_Silicon.zip` **no está incluido directamente en el repositorio**: un `.app` para macOS solo arranca en Apple Silicon si está firmado, y eso requiere compilarlo en un Mac real — no se puede firmar código de Apple desde Linux. Consíguelo así:

1. Ve a la pestaña **[Actions](../../actions)** de este repositorio → abre la ejecución más reciente del workflow *Build* → descarga el artefacto **`SantJoanTetris-macOS`** (compilado y firmado en un Mac real por el propio workflow).
2. Descomprime el `.zip` y haz doble clic en **`SantJoanTetris.app`**.
3. *Nota Gatekeeper:* aunque la app ya viene firmada (firma ad-hoc), al descargarla del navegador macOS le añade una "cuarentena". La primera vez, haz clic derecho sobre la app y pulsa **Abrir** (o en Terminal: `xattr -cr SantJoanTetris.app`).

Si prefieres compilarlo tú mismo en tu propio Mac: `make package-mac` (necesita `brew install sdl2`).

---

## 🎮 Controles del Turno

| Tecla | Acción |
|---|---|
| <kbd>←</kbd> / <kbd>→</kbd> o <kbd>A</kbd> / <kbd>D</kbd> | Mover paquete de datos a izquierda / derecha |
| <kbd>↑</kbd> o <kbd>W</kbd> | Rotar en sentido horario (CW) |
| <kbd>Z</kbd> o <kbd>Q</kbd> | Rotar en sentido antihorario (CCW) |
| <kbd>↓</kbd> o <kbd>S</kbd> | Caída suave (*Soft Drop*) |
| <kbd>Espacio</kbd> | Caída instantánea (*Hard Drop*) |
| <kbd>C</kbd> o <kbd>Shift</kbd> | Guardar en la **Caché (Hold)** |
| <kbd>F11</kbd> o <kbd>Alt</kbd>+<kbd>Enter</kbd> | Pantalla completa / Maximizar ventana |
| <kbd>T</kbd> | Cambiar tema musical de guardia |
| <kbd>M</kbd> | Silenciar / Activar música |
| <kbd>+</kbd> / <kbd>-</kbd> | Regular volumen general |
| <kbd>P</kbd> o <kbd>Escape</kbd> | Pausa para ir a por café |
| <kbd>R</kbd> | Reiniciar turno de guardia |
| <kbd>F1</kbd> o <kbd>H</kbd> | Manual de supervivencia del informático de guardia |
| <kbd>F12</kbd> | Guardar captura de pantalla (`screenshot_*.bmp`) |

---

## 🏆 Récord de Guardia

Las puntuaciones más altas quedan guardadas de forma persistente en `highscore.txt`.

---

## 🛠️ Compilar desde el código fuente

Todo el juego nativo (Linux/Windows/Mac) es un único código C compartido en `src/`, sin dependencias más allá de SDL2. La versión Web (`SantJoanTetris.html`) es una reimplementación en JavaScript/canvas que replica el mismo diseño visual (misma fuente de píxeles, paneles y bloques) y el mismo motor de audio, portados directamente del código C, para que funcione en cualquier navegador sin instalar nada.

```bash
make linux          # binario nativo de Linux
make windows         # SantJoanTetris.exe (cross-compilado con zig)
make mac             # binario de macOS (nativo si se ejecuta en un Mac; cross-compilado si no)
make mac-app         # empaqueta el .app y lo firma si hay `codesign` disponible
make package-windows # SantJoanTetris_Windows.zip listo para repartir
make package-mac     # SantJoanTetris_macOS_Silicon.zip (firmado, si se ejecuta en un Mac)
make package-web     # SantJoanTetris_Web.zip (el .html + su LEEME)
make package-all     # los tres paquetes anteriores de una vez
make all             # windows + linux + mac
make clean           # borra todos los binarios y paquetes generados
```

Requiere [zig](https://ziglang.org/) 0.13.0 para los targets de Windows y (si se cruza desde Linux) Mac — hay un `mise.toml` en el repo que lo fija automáticamente si usas [mise](https://mise.jdx.dev/).

### CI: la forma fiable de obtener los binarios reales

`.github/workflows/build.yml` compila las cuatro versiones en cada push:
Linux y Windows en un runner de Ubuntu, **macOS en un runner de Apple real**
(`macos-latest`, compilación nativa + firma ad-hoc genuina), y una
comprobación de humo de la versión Web con un navegador real sin errores de
consola. Los artefactos de cada build quedan descargables desde la pestaña
*Actions* del repositorio — es la forma más fiable de conseguir un `.app` de
Mac que funcione, ya que aquí no hay manera de firmar código de Apple fuera
de un Mac de verdad.
