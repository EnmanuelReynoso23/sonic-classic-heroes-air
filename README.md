# 🦔 Sonic Classic Heroes AIR

**Sonic Classic Heroes AIR** es un proyecto modular de código abierto para **Sonic 3 A.I.R. (Angel Island Revisited)** que recrea la legendaria experiencia de *Sonic Classic Heroes*:
- **3 personajes simultáneos en pantalla:**
  - **Team Sonic:** Sonic (Velocidad), Tails (Vuelo) y Knuckles (Fuerza).
  - **Team Chaotix:** Espio (Velocidad), Charmy (Vuelo) y Vector (Fuerza).
- **Tag System (Intercambio en tiempo real):** Alterna el control del líder activo con solo presionar un botón (`Y` / `Triángulo` / `X`).
- **IA de formación avanzada:** Los compañeros siguen al líder, saltan juntos, vuelan en asistencia y no se quedan atascados.
- **Soporte de niveles:** Preparado para integrar las zonas de Sonic 1, Sonic 2 y la campaña completa de Sonic 3 & Knuckles a 60 FPS en pantalla panorámica (16:9).

---

## 🚀 Cómo Instalar en Sonic 3 A.I.R.

1. **Descarga Sonic 3 A.I.R.:**
   - Si aún no lo tienes, descárgalo gratis desde la web oficial: [sonic3air.org](https://sonic3air.org/)
   - Requiere la ROM original de *Sonic 3 & Knuckles* (`Sonic_Knuckles_wSonic3.bin`).

2. **Copia esta carpeta al directorio de Mods:**
   - En Windows, presiona `Win + R` y escribe:
     ```text
     %APPDATA%\Sonic3AIR\mods\
     ```
   - O si usas la versión portátil, en la subcarpeta:
     ```text
     Sonic3AIR/data/mods/
     ```
   - Pega la carpeta `sonic-classic-heroes-air` completa ahí.

3. **Activar el Mod en el juego:**
   - Abre *Sonic 3 A.I.R.*.
   - Ve a **Options -> Mods**.
   - Busca **"Sonic Classic Heroes AIR"** y actívalo.

---

## 🎮 Controles

| Acción | Mando (Xbox / PlayStation) | Teclado (Por Defecto) |
| :--- | :--- | :--- |
| **Moverse** | Cruceta / Stick Izquierdo | Flechas de dirección |
| **Saltar** | `A` / `Cruz` | `Espacio` / `Z` |
| **Habilidad Especial** | Doble salto en el aire | Doble salto en el aire |
| **Cambiar Líder (Tag)** | `Y` / `Triángulo` o `X` / `Cuadrado` | Tecla `S` o `D` |

---

## 🛠️ Estructura del Código Fuente (Lemon Script)

```text
sonic-classic-heroes-air/
├── mod.json                     # Metadatos del mod y opciones configurables
├── README.md                    # Documentación y guía de instalación
└── scripts/
    ├── main.lemon               # Punto de entrada y hooks globales de inicio
    ├── character_swap.lemon     # Lógica del Tag System y alternancia de cámaras
    ├── heroes_ai.lemon          # IA de seguimiento, formaciones y saltos de los compañeros
    └── heroes_hud.lemon         # HUD con los retratos y el líder activo
```

---

## 💡 Modo Desarrollador (Live Reload)
Para ver los mensajes de depuración en vivo del compilador Lemon Script:
```powershell
.\Sonic3AIR.exe --developer
```
Cualquier cambio que guardes en los archivos `.lemon` se compilará automáticamente en tiempo real sin reiniciar el juego.
