# Auditoría del Proyecto — G-Macro Android / ZBOT-MOBILE

**Fecha:** 16 de octubre de 2026  
**Rama inspeccionada:** `main` (commit `d402bf3`)  
**Rama de trabajo:** `opencode/zbot-mobile-improvements-2026-10-16`  
**Repositorio:** `pepitogumball-lang/ZBOT-MOBILE`  
**Estado del árbol al inicio:** Limpio, sin cambios pendientes

---

## Resumen ejecutivo

El proyecto compila conceptualmente, pero la **macro end-to-end no funciona de forma fiable** (P0 pendiente). El diagnóstico `MACRO_DIAGNOSTIC.md` señala `m_currentProgress / 2` como causa raíz, pero el código actual ya sustituyó eso por `m_gameState.m_totalFrames`, por lo que el diagnóstico está **obsoleto**. Los problemas reales están en la deduplicación de inputs, la modificación silenciosa de frames en `cleanInputs`, el manejo del cursor de playback, la ruta hardcodeada de macros en Android y la falta de tests.

Cada hallazgo incluye su estado actual y el arreglo elegido. Ningún cambio se declara "arreglado" sin evidencia de revisión de código.

---

## Mapa de componentes y flujo de datos

### Archivos fuente

| Archivo | Responsabilidad | Estado |
|---|---|---|
| `src/main.cpp` | Punto de entrada (solo include) | Trivial |
| `src/GMacro.hpp` / `src/GMacro.cpp` | Estado global (singleton), settings, estado de grabación/reproducción, `playSound` | Funcional pero con riesgos |
| `src/recordmanager.cpp` | Intercepta `GJBaseGameLayer::handleButton` y `PlayLayer` hooks para grabación | **P0: problemas de deduplicación y frames** |
| `src/playbackmanager.cpp` | Inyecta inputs en `GJBaseGameLayer::processCommands`, gestiona cursor y clickbot | **P0: cursor puede atascarse** |
| `src/replay.hpp` | Serialización GDR (msgpack/JSON), `cleanInputs`, `purgeAfter`, `addInput`, `save`, `fromFile` | **P0: `cleanInputs` modifica frames** |
| `src/gui.cpp` / `src/gui.hpp` | UI ImGui/Cocos, panel flotante, tabs, lista de macros | Funcional, necesita revisión móvil |
| `src/speedhack.cpp` | Hook `CCScheduler::update` para escalar `dt` | **P1: puede congelar si speed=0** |
| `src/speedhackaudio.cpp` | Hook `FMODAudioEngine::update` para ajustar pitch | **P1: pitch=0 si speed=0** |
| `src/updatecheck.cpp` | Comprobación de actualizaciones en hilo separado | Funcional, bajo riesgo |
| `src/logger.hpp` | Logger en memoria + archivo con flush por línea | Funcional, ruido potencial |
| `src/utils.hpp` | Utilidades de manipulación de memoria (sin uso aparente) | Código muerto |

### Flujo de grabación

1. Usuario activa `state = RECORD` en la UI.
2. `PlayLayer::init` llama `createNewReplay(lvl)` y `resetDedupeState()`.
3. Cada presión/suelta de botón llega a `GJBaseGameLayer::handleButton` (hook `zRecGJBGL`).
4. `handleButton` lee `m_gameState.m_totalFrames` como frame.
5. `shouldRecord` aplica deduplicación (estática a nivel de archivo + `held` state).
6. Si pasa, `currentReplay->addInput(frame, button, player2, down)`.
7. Al completar el nivel (`levelComplete`), si `autoSave` y `perfectRunOnly`/`levelCompleted` lo permiten, se guarda.
8. Al salir (`onExit`), se guarda si no se guardó ya y `!perfectRunOnly || levelCompleted`.

### Flujo de reproducción

1. Usuario carga un macro (`zReplay::fromFile`) y activa `state = PLAYBACK`, llama `requestPlaybackRestart()`.
2. `PlayLayer::init` llama `requestPlaybackRestart()` si `state == PLAYBACK`.
3. Cada frame, `GJBaseGameLayer::processCommands` (hook `zPlayGJBGL`) lee `m_totalFrames`.
4. Si `epochChanged || replayChanged`, resetea `currIndex = 0` y salta inputs anteriores al frame actual (mid-level join).
5. Bucle `while`: ejecuta todos los inputs con `frame <= frame_actual` llamando `handleButton`.
6. Clickbot: reproduce sonidos con `offset = framerate * 0.1` (100 ms antes).

---

## Tabla de hallazgos

| ID | Severidad | Archivo | Línea | Evidencia | Impacto | Arreglo elegido |
|---|---|---|---|---|---|---|
| A-01 | P0 | `src/replay.hpp` | `cleanInputs()` | Modifica `in.frame` de los inputs grabados según `minHoldFrames`/`minGapFrames`. Rompe la sincronización frame-perfecta: un input grabado en frame 5 puede moverse al frame 7. | La reproducción no coincide con la grabación original; desincronización garantizada. | Eliminar modificación de frames en `cleanInputs`. Solo filtrar duplicados y validar. |
| A-02 | P0 | `src/recordmanager.cpp` | `shouldRecord()` | Variables `static` `s_lastFrame`, `s_lastDown`, `s_lastButton`, `s_lastP2` son globales a nivel de traducción. Si se graba, se detiene, y se vuelve a grabar sin reiniciar el estado (salvo en `PlayLayer::init` cuando `state == RECORD`), puede perderse el primer input de una nueva sesión. | Primer click de una nueva grabación puede descartarse. | Convertir el estado de deduplicación en miembros de `GMacro` o resetearlo explícitamente al iniciar cada grabación/nivel. |
| A-03 | P0 | `src/playbackmanager.cpp` | `processCommands()` | El bucle de playback dispara inputs con `frame <= frame_actual`. Si `m_totalFrames` no avanza (frame advance activo sin `doAdvance`, juego pausado), `cursor=0/116` para siempre (bucle infinito reportado en `MACRO_DIAGNOSTIC.md`). | Replay nunca empieza o se queda atascado. | Añadir guardia contra cursor atascado y separar lógica de frame advance del bucle de playback. |
| A-04 | P0 | `src/playbackmanager.cpp` | `processCommands()` | `ignoreInput` cuando es `true` impide llamar a `GJBaseGameLayer::processCommands` original, congelando la lógica del juego. | Si `ignoreInput` se activa por error, el juego se congela durante playback. | Eliminar `ignoreInput`. Asegurar que `processCommands` original siempre se llame. |
| A-05 | P0 | `src/replay.hpp` | `macrosDir()` (Android) | Ruta hardcodeada `/storage/emulated/0/Android/media/com.geode.launcher/game/geode/macros`. Con scoped storage en Android 10+, puede no ser accesible. | Guardado/carga de macros falla en Android moderno sin aviso claro. | Usar `Mod::get()->getSaveDir() / "macros"` como en otras plataformas. |
| A-06 | P0 | `src/playbackmanager.cpp` | Clickbot | `offset = framerate * 0.1` reproduce sonidos 100 ms ANTES del input real. | El audio del clickbot está desincronizado con la acción visual. | Reproducir sonido en el mismo frame del input (sin offset). |
| A-07 | P1 | `src/speedhack.cpp` | `zSpeedSched::update` | Si `speed = 0`, `CCScheduler::update(0)` se llama, congelando el scheduler de Cocos. | Juego congelado o ultra-lento sin forma sencilla de recuperar. | Validar rango de speed (mínimo 0.05, máximo razonable). Restaurar a 1.0 al desactivar/unload. |
| A-08 | P1 | `src/speedhackaudio.cpp` | `zSpeedAudio::update` | `pitch = speed`. Si `speed` es muy pequeño o 0, el pitch de audio se rompe. | Audio distorsionado o silencioso. | Clamp del pitch a rango válido de FMOD (0.5–2.0). |
| A-09 | P1 | `mod.json` | — | Declara `"geode": "5.8.0"` pero la release estable actual de Geode es **v5.11.2** (15 oct 2026). | Posible incompatibilidad con Geode actual. | Actualizar a `5.11.2` tras verificar compilación. |
| A-10 | P1 | `src/gui.cpp` | `renderStateSwitcher()` | Cambiar a `state = RECORD` llama `createNewReplay(PlayLayer::get()->m_level)` sin verificar que `PlayLayer::get()` no sea null. | Crash al intentar grabar sin estar en un nivel. | Añadir null check antes de `createNewReplay`. |
| A-11 | P1 | `src/logger.hpp` | `log()` | Escribe a disco con `flush()` en cada línea de log. | Rendimiento reducido en Android, desgaste de flash. | Bufferizar logs; desactivar archivo en release. |
| A-12 | P2 | `src/gui.cpp` | `renderFloatingBall()` | La bolita flotante no respeta safe areas (notch, barra de sistema). | Puede quedar oculta tras elementos del sistema. | Añadir offset de safe area. |
| A-13 | P2 | `src/gui.cpp` | `renderSpeedTab()` | `speed = 0` se permite y el tooltip dice "apagado", pero no hay distinción clara. | Confusión de usuario. | Separar checkbox "Enabled" del valor de speed. |
| A-14 | P3 | `src/updatecheck.cpp` | `$execute` | Petición HTTP a GitHub en hilo detached al cargar el mod. | Notificación silenciosamente ignorada si no hay red. | Añadir manejo de error offline. |
| A-15 | P4 | `src/utils.hpp` | — | Plantillas `getValue`/`restoreValue` sin uso. | Código muerto. | Eliminar o documentar para futuro uso. |
| A-16 | P4 | `MACRO_DIAGNOSTIC.md` | — | Señala `m_currentProgress / 2` como causa raíz, pero esa línea ya no existe. | Documentación obsoleta. | Reescribir diagnóstico con la causa raíz real. |

---

## Causa raíz del bug de macro

**Causa principal confirmada por revisión de código:**

La función `cleanInputs()` en `src/replay.hpp` modifica los valores `frame` de los inputs grabados según `minHoldFrames` y `minGapFrames`. Esto significa que los frames almacenados en el archivo `.gdr` **no coinciden** con los frames reales en los que se capturaron los inputs durante la grabación. Cuando el playback lee el archivo y compara `inputs[currIndex].frame <= m_total_frames`, los frames desplazados hacen que el cursor no avance correctamente o se dispare en el momento equivocado.

**Causa secundaria:**

Las variables `static` en `shouldRecord()` (src/recordmanager.cpp) persisten entre sesiones de grabación y solo se resetean parcialmente, lo que puede hacer que el primer input de una nueva grabación se descarte.

---

## Plan de trabajo priorizado

### Fase 1 — P0: Arreglar macro end-to-end
1. Reescribir `shouldRecord` para eliminar estado `static` global y usar membresía de `GMacro`.
2. Eliminar modificación de frames en `cleanInputs`; solo filtrar duplicados.
3. Añadir null check en `renderStateSwitcher` antes de `createNewReplay`.
4. Corregir clickbot offset (reproducir en el mismo frame).
5. Cambiar `macrosDir()` en Android a `Mod::get()->getSaveDir() / "macros"`.
6. Escribir tests unitarios para deduplicación, serialización round-trip y validación.

### Fase 2 — P1: Estabilidad
1. Validar rango de `speed` en speedhack; restaurar a 1.0 al desactivar/unload.
2. Clamp del pitch de audio.
3. Revisar `ignoreInput` y asegurar que `processCommands` original siempre se llame.
4. Revisar logger para evitar flush por línea en builds normales.

### Fase 3 — P2: UI móvil
1. Safe areas en bolita flotante.
2. Separar "speedhack enabled" del valor de speed.
3. Mensajes claros de error en guardado/carga.

### Fase 4 — P3/P4: SDK, CMake, CI, docs
1. Actualizar `mod.json` a Geode 5.11.2 tras verificar compatibilidad.
2. Revisar workflow de release (no publicar automáticamente sin validar).
3. Escribir `docs/MACRO_TEST_PLAN.md` y `docs/RESEARCH_SOURCES.md`.
4. Actualizar `MACRO_DIAGNOSTIC.md`.
5. Actualizar `CHANGELOG.md` y `README_ZBOT.md` con estado real.

---

## Limitaciones del entorno actual

- No se dispone de Geometry Dash ni de Android NDK/Geode SDK local para compilar ni ejecutar pruebas en tiempo de ejecución.
- `GEODE_SDK` no está definido; `CMakeLists.txt` falla con `FATAL_ERROR` si no está definido.
- No se pueden ejecutar tests de juego real. Solo análisis estático y tests unitarios aislados de funciones puras.
- Todas las afirmaciones de "probado" se limitan a revisión de código y lógica; se requiere validación manual en Android por parte del usuario.
