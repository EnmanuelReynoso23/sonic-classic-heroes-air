# Sonic Classic Heroes AIR

Mod para **Sonic 3 A.I.R.** que continúa la historia de *Sonic Classic Heroes* (Sonic 1 + Sonic 2) en **Sonic 3 & Knuckles**: Sonic, Tails y Knuckles recorren juntos todas las zonas, de Angel Island a Death Egg, y cambias de líder cuando quieras, como en el juego original de Flamewing.

## Qué hace

- **Tres personajes en pantalla.** El líder es el jugador de verdad: tiene la física, las habilidades y las interacciones originales del personaje. Los otros dos lo siguen repitiendo su recorrido con unos fotogramas de retraso, con los sprites, la paleta y las animaciones originales del juego.
- **Cambio de líder en cualquier momento** (Speed → Fly → Power):
  - **Sonic (Speed):** Spin Dash, Insta-Shield, Drop Dash y los escudos elementales.
  - **Tails (Fly):** vuela y nada. Con *Vuelo en equipo*, Sonic y Knuckles van colgados de él.
  - **Knuckles (Power):** planea, trepa y **rompe las paredes y rocas exclusivas de Knuckles**.
- **Cambio en el aire:** de Tails volando a Sonic, o de Sonic saltando a Tails para seguir volando. Hasta dos cambios por salto, para que no se pueda volar sin fin.
- **Formación:** cuando el líder se detiene, el equipo se pone en fila detrás de él y camina a su lugar. Si se da la vuelta, los demás cruzan al otro lado caminando.
- **Indicador del equipo** debajo de los anillos (el líder sale en el color de su formación), y el contador de vidas muestra al líder actual.
- **La historia es la del personaje que elijas en el Data Select.** En las escenas de la historia el líder vuelve a ser ese personaje, y después se recupera el que tenías.

## Instalación

1. Instala Sonic 3 A.I.R. desde [sonic3air.org](https://sonic3air.org) o la [página de releases](https://github.com/Eukaryot/sonic3air/releases).
   - Necesitas **tu propia ROM** de *Sonic 3 & Knuckles* (`Sonic_Knuckles_wSonic3.bin`, por ejemplo la de Steam). El juego la pide al abrirlo por primera vez.
2. Copia esta carpeta en `%APPDATA%\Sonic3AIR\mods\`, o ejecuta:
   ```
   python install_mod_to_s3air.py
   ```
   Así se crea un enlace a esta carpeta y los cambios se ven al instante.
   `python download_and_launch_s3air.py` además descarga el juego si no lo tienes y lo abre.
3. En el juego: **menú principal → Mods**, y activa *Sonic Classic Heroes AIR*.
4. Empieza una partida con cualquier personaje. Si eliges *Sonic & Tails*, se juega como Sonic solo, porque el equipo ya trae a su propio Tails.

## Controles

| Acción | Botón |
|---|---|
| Siguiente líder (Sonic → Tails → Knuckles) | **R**, o **Y** en el suelo |
| Líder anterior | **L** |

**Y** en el aire sigue sirviendo para la transformación en Super. Los botones L y R se asignan en *Options → Controls* (el mod los activa con `Controls_LR`).

## Opciones

En *Options → Mods → Classic Heroes*:

| Opción | Valores |
|---|---|
| Modo Classic Heroes | Activado / Desactivado |
| Cambiar de líder | R/L y Y en el suelo · Solo R/L · R/L y Y también en el aire |
| Distancia del equipo | Muy cerca · Normal · Lejos |
| Vuelo en equipo | Activado / Desactivado |
| Recordar el líder | Sí, también entre zonas · No, empezar con el de la historia |
| Indicador del equipo | Visible / Oculto |

## Cómo funciona

| Archivo | Contenido |
|---|---|
| `scripts/heroes/team.lemon` | Cambio de líder: convierte al jugador 1 (`0xffffb000`) en otro personaje sin cortar la física (dirección de actualización, mappings, hitbox, límites de velocidad, cola de Tails, polvo, Insta-Shield, cámara). |
| `scripts/heroes/trail.lemon` | Guarda en cada fotograma dónde estaba el líder y qué hacía, en un búfer circular de 64 entradas en la memoria compartida (`0x830000`). |
| `scripts/heroes/followers.lemon` | Los compañeros: objetos falsos a los que se aplican las propias funciones de animación del juego (`UpdateSonicAnimation`, etc.). Incluye la fila al detenerse y el vuelo en equipo. |
| `scripts/heroes/hud.lemon` | Indicador del equipo e ícono de vidas. |
| `scripts/heroes/hooks.lemon` | Puntos donde el mod se engancha al juego: selección de personajes, inicio de nivel, actualización del jugador 1, dibujado de sprites, vuelo y muerte de Tails cuando es el líder. |

## Pruebas

Sin ROM el juego no carga scripts, así que `tools/lemoncheck` compila los scripts originales del juego más el mod con el mismo compilador de Lemon Script de la versión v26.03.28. Además puede ejecutar el mod con la memoria simulada.

```
tools/lemoncheck.sh check   # compila (Linux o WSL: necesita g++, cmake y zlib)
tools/lemoncheck.sh test    # compila y ejecuta tests/heroes_test.lemon
```

Las pruebas comprueban varias cosas, fotograma a fotograma:

- La fila al estar quieto y el seguimiento a 8 y 16 fotogramas de retraso al correr.
- Que nadie adelanta al líder al frenar y que, al darse la vuelta, cruzan caminando.
- El cambio de líder en el suelo y en el aire: hitbox, posición de los pies, cola de Tails y polvo.
- La espera entre cambios y el límite de cambios por salto.
- El vuelo en equipo.
- Las transiciones de acto, y que no haya dos Tails cuando el juego usa el suyo.
- La paleta y el orden de dibujado de cada compañero.
- El vuelo y la muerte de Tails cuando es el líder.
- Que la pila de registros quede equilibrada.

## Limitaciones actuales

- Los compañeros **acompañan, pero no interactúan**: no golpean enemigos, no recogen anillos y no reciben daño. Todo eso lo hace el líder.
- Super/Hyper Tails solo funciona si Tails es el personaje de la historia (así lo limita el juego). Mientras haya una transformación Super activa no se puede cambiar de líder.
- No se puede cambiar de líder mientras Knuckles planea o trepa, durante un Spin Dash, en escenas, ni en Doomsday, las etapas bonus ni el modo competición.
- Al ponerse en fila en una pendiente, los compañeros pueden quedar unos píxeles por encima o por debajo del suelo.
- **Team Chaotix** (Espio, Charmy, Vector) todavía no existe. Hacen falta sus sprites, y Sonic 3 A.I.R. solo permite reemplazar sprites existentes. El gancho `Standalone.getModdedAnimationSpriteKey` está listo para cuando haya arte.
- **Las zonas de Sonic 1 y Sonic 2 no están incluidas.** Sonic 3 A.I.R. solo carga niveles de S3&K. Esa parte de la historia es *Sonic Classic Heroes* original, y este mod la continúa en S3&K.

## Créditos

- *Sonic Classic Heroes*: Flamewing y ColinC10.
- *Sonic 3 A.I.R.* y el Oxygen Engine: Eukaryot ([GPLv3](https://github.com/Eukaryot/sonic3air)).
- *Sonic the Hedgehog* © SEGA. Este mod no incluye la ROM ni datos del juego.
