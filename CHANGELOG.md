# Cambios de G-Macro Android

### v1.8.0
* **FIX CRÍTICO (P0):** `cleanInputs()` en `replay.hpp` ya no modifica los frames de los inputs grabados. Antes desplazaba `in.frame` según `minHoldFrames`/`minGapFrames`, rompiendo la sincronización frame-perfecta entre grabación y reproducción.
* **FIX CRÍTICO (P0):** El estado de deduplicación (`shouldRecord`) ahora usa miembros de `GMacro` en lugar de variables `static` de archivo, eliminando la pérdida del primer input en nuevas sesiones de grabación.
* **FIX CRÍTICO (P0):** Clickbot ahora reproduce el sonido en el mismo frame del input (sin offset de 100 ms), sincronizando audio y acción visual.
* **FIX CRÍTICO (P0):** `macrosDir()` en Android usa `Mod::get()->getSaveDir() / "macros"` en lugar de una ruta hardcodeada, compatible con scoped storage.
* **FIX CRÍTICO (P0):** Guardia contra cursor de playback atascado: resetea automáticamente tras 120 frames sin progreso.
* **FIX:** `ignoreInput` eliminado; `processCommands` original siempre se ejecuta para evitar congelar el juego.
* **FIX:** `renderStateSwitcher` ahora verifica que `PlayLayer::get()` y `m_level` no sean null antes de crear un replay.
* **FIX:** Speedhack valida rango de velocidad (mínimo 0.05, máximo 8.0) para evitar congelar el scheduler de Cocos.
* **FIX:** Pitch de audio de speedhack acotado a rango válido [0.5, 2.0] para evitar audio distorsionado.
* **MEJORA:** Logger bufferiza escrituras en disco (flush cada 10 líneas) para reducir I/O en Android.
* **MEJORA:** Bolita flotante respeta safe areas (margen mínimo de 8px en bordes).
* **MEJORA:** `mod.json` actualizado a Geode 5.11.2 (release estable actual).
* **DOCS:** Reescrito `MACRO_DIAGNOSTIC.md` con causa raíz real; el diagnóstico anterior estaba obsoleto.
* **DOCS:** Añadidos `docs/PROJECT_AUDIT.md`, `docs/MACRO_TEST_PLAN.md`, `docs/RESEARCH_SOURCES.md`.

### v1.7.0
* Renombrado el proyecto a G-Macro Android.
* Actualizada la identidad del mod y el ID interno para evitar conflictos.
* Limpieza general de código y profesionalización de comentarios.
* Mejora en la transparencia de créditos técnicos.

### v1.6.1
* Arreglado un bug tonto con los frames que hacía que a veces no grabara nada.
* María arregló lo del auto-save que a veces decía que guardaba y luego no.
* Limpieza general de código antes de subirlo.
* Actualizado para que vaya bien con el último Geode.
* Arreglado el selector de jugador en el spam (spamPlayer ya funciona bien en dual mode).
* Todos los textos de la UI ahora en español, que era lo que tocaba.
* El update check ya usa la versión del mod.json en vez de tenerla hardcodeada dos veces.
* Eliminada la configuración de Replit que no pintaba nada en el repo.
* Corregido el fallo del build en GitHub Actions (faltaban archivos en el repo remoto).
* Sincronizados todos los archivos fuente (.cpp y .hpp) que se habían perdido en el historial.

### v1.6.0
* Metimos lo del Frame Advance para los que quieren hacer TAS.
* Arreglamos los desincronizados en niveles rápidos.
* Alberto cambió un par de cosas de la UI para que se vea mejor en tablets.

### v1.5.0
* Primera versión estable que no crasheaba cada 5 minutos.
* El speedhack ya funciona con audio.
* Añadido el clickbot básico.
