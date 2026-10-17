# Cambios de G-Macro Android

### v1.8.0
* **FIX BLOQUEANTE (build):** El mod **no compilaba** desde el commit `d402bf3`. Se usaba `GJGameState::m_totalFrames`, un miembro que **no existe** en los bindings de GD 2.2081. Se corrigió usando `GJBaseGameLayer::m_currentStep`. Sin este arreglo nada funcionaba: el mod ni siquiera cargaba. Verificado por compilación real en CI.
* **FIX CRÍTICO (P0):** `cleanInputs()` en `replay.hpp` ya no modifica los frames de los inputs grabados. Antes desplazaba `in.frame` según `minHoldFrames`/`minGapFrames`, rompiendo la sincronización frame-perfecta entre grabación y reproducción.
* **FIX CRÍTICO (P0):** El estado de deduplicación (`shouldRecord`) ahora usa miembros de `GMacro` en lugar de variables `static` de archivo, eliminando la pérdida del primer input en nuevas sesiones de grabación.
* **FIX (P1):** `shouldRecord` mantiene siempre el estado de botones (`p1ButtonHeld`/`p2ButtonHeld`), incluso con la deduplicación desactivada, para no perder los "release" al reiniciar el nivel.
* **FIX CRÍTICO (P0):** Clickbot reproduce el sonido en el mismo frame del input (sin el antiguo offset de 100 ms).
* **FIX CRÍTICO (P0):** `macrosDir()` usa `Mod::get()->getSaveDir() / "macros"` en lugar de una ruta hardcodeada, compatible con scoped storage en Android 10+.
* **FIX (P1):** El cursor de playback atascado ahora es solo un **diagnóstico** (se registra en el log). Ya **no** se auto-resetea: hacerlo re-ejecutaba inputs ya pasados.
* **FIX:** `ignoreInput` eliminado; `processCommands` original siempre se ejecuta para evitar congelar el juego.
* **FIX:** `renderStateSwitcher` verifica que `PlayLayer::get()` y `m_level` no sean null antes de crear un replay.
* **FIX:** `replay::fromFile` valida el nombre (anti path-traversal) y el tamaño, y envuelve `importData` en try/catch: un `.gdr` corrupto ya no puede tirar el juego.
* **FIX:** `replay::save` comprueba `GJAccountManager::get()` y rechaza nombres vacíos.
* **FIX:** Speedhack acota la velocidad a `[0.05, 8.0]`; la UI usa los mismos límites (ya no promete "sin límite").
* **FIX:** Pitch de audio del speedhack acotado a `[0.5, 2.0]`.
* **FIX:** El spam reinicia su fase al pausarse con "Only while playing" activo, evitando saltos al reanudar.
* **MEJORA:** Lógica pura de macros extraída a `src/macro_utils.hpp` (sin dependencias de Geode) y cubierta por tests unitarios en `tests/`.
* **MEJORA:** Nuevo job de CI "Unit tests" que compila y ejecuta esos tests en cada build.
* **MEJORA:** El workflow ya **no** publica releases en builds manuales (`workflow_dispatch`); solo publica en push a `main`.
* **MEJORA:** Logger bufferiza escrituras a disco (flush cada 10 líneas) y usa `localtime_r` (thread-safe).
* **MEJORA:** Bolita flotante respeta safe areas (margen mínimo de 8px en bordes).
* **MEJORA:** `mod.json` actualizado a Geode 5.11.2.
* **LIMPIEZA:** Eliminado `src/utils.hpp` (código muerto, nunca incluido ni listado en CMake).
* **DOCS:** Reescrito `MACRO_DIAGNOSTIC.md` con la causa raíz real; el diagnóstico anterior estaba obsoleto.
* **DOCS:** Añadidos `docs/PROJECT_AUDIT.md`, `docs/MACRO_TEST_PLAN.md`, `docs/RESEARCH_SOURCES.md`.

> **Estado de validación:** todo lo anterior está respaldado por compilación real en CI
> (Android32 + Android64) y por tests unitarios automáticos. El comportamiento **en juego**
> (semántica de `m_currentStep` al grabar vs reproducir, checkpoints y reinicios) queda
> **pendiente de validación manual en un dispositivo Android** — ver `docs/MACRO_TEST_PLAN.md`.

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
