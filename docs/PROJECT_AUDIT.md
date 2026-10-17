# Auditoría del Proyecto — G-Macro Android / ZBOT-MOBILE

**Fecha:** 17 de octubre de 2026  
**Rama inspeccionada:** `main` (commit `d402bf3`)  
**Rama de trabajo:** `opencode/zbot-mobile-improvements-2026-10-16`  
**Repositorio:** `pepitogumball-lang/ZBOT-MOBILE`  
**Estado del árbol al inicio:** Limpio, sin cambios pendientes

---

## Resumen ejecutivo

**El proyecto NO compilaba en `main`.** El commit `d402bf3` ("Fix macro stability by
using totalFrames") introdujo el miembro `m_gameState.m_totalFrames`, que **no existe**
en los bindings de GD 2.2081 (el campo real es `m_currentStep` en `GJBaseGameLayer`, y
`m_totalTime`/`m_currentProgress` en `GJGameState`). Como consecuencia, cada unidad de
traducción fallaba al compilar y el CI llevaba **rojo desde ese commit**. Esto se verificó
con el log real de GitHub Actions (ver A-00), no por suposición.

Una vez corregido el build, se abordaron los problemas de la macro end-to-end. El
diagnóstico `MACRO_DIAGNOSTIC.md` original señalaba `m_currentProgress / 2` como causa
raíz, pero esa hipótesis nunca se confirmó y el código ya la había sustituido. Los
problemas reales (tras restaurar el build) están en: el miembro inexistente para
calcular frames, la modificación silenciosa de frames en `cleanInputs`, el estado de
deduplicación `static` que se filtra entre sesiones, el offset de clickbot, la ruta
hardcodeada de macros en Android y la falta de tests.

Cada hallazgo incluye su estado actual y el arreglo elegido. Ningún cambio se declara
"arreglado" sin evidencia de compilación o revisión de código; el comportamiento en
juego queda **pendiente de validación manual** (ver `docs/MACRO_TEST_PLAN.md`).

---

## Evidencia de compilación (CI)

| Ejecución | Commit | Rama | Resultado |
|---|---|---|---|
| `29138154381` | `d402bf3` | `main` | ❌ failure (log expirado, 410) |
| `29006856617` | `7aec330` | work branch | ❌ failure — llave faltante en `cleanInputs` + `m_totalFrames` |
| `39007179561` | `094cc07` | work branch | ❌ failure — `m_totalFrames` (causa raíz) |
| `39007462340` | `2e417e6` | work branch | ✅ **success** (Android32 + Android64 + package) |
| `39007940379` | `90e980b` | work branch | ✅ **success** (+ job **Unit tests**) |

**Importante (proceso):** el job `Publish Release` del workflow corría también en
`workflow_dispatch`, de modo que un build manual desde una rama de trabajo llegó a
publicar un release `v1.8.0`. Se corrigió el workflow para que el release **solo** se
publique en push a `main`; en el run `39007940379` el job aparece como `skipped`. No se
borró ni sobrescribió ningún release o tag existente.

La causa raíz del build roto se leyó directamente del log del run `39007179561`:

```
src/playbackmanager.cpp:40:46: error: no member named 'm_totalFrames' in 'GJGameState';
did you mean 'm_totalTime'?
src/recordmanager.cpp:54:52: error: no member named 'm_totalFrames' in 'GJGameState'
src/gui.cpp:301:55: error: no member named 'm_totalFrames' in 'GJGameState'
```

Verificación de los bindings reales (2.2081): `GJBaseGameLayer` tiene `int m_currentStep;`
y `GJGameState` tiene `double m_totalTime;`, `unsigned int m_currentProgress;`, pero
**ningún** `m_totalFrames`. GD usa `int m_step` en su propio `PlayerButtonCommand`, y
otros mods de replay (`GatoBot`, `GD-Replay-Mod`) usan `layer->m_currentStep`.

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
| A-00 | **P0 (bloqueante)** | `src/recordmanager.cpp`, `src/playbackmanager.cpp`, `src/gui.cpp` | cálculo de `frame` | `m_gameState.m_totalFrames` **no existe** en los bindings de GD 2.2081. El compilador lo confirmó: *"no member named 'm_totalFrames' in 'GJGameState'; did you mean 'm_totalTime'?"*. Todo el mod dejó de compilar desde `d402bf3`. | Build roto: el mod no carga en absoluto. | Usar `GJBaseGameLayer::m_currentStep` (el contador canónico, igual que GD internamente y otros mods). |
| A-01 | P0 | `src/replay.hpp` | `cleanInputs()` | Modifica `in.frame` de los inputs grabados según `minHoldFrames`/`minGapFrames`. Rompe la sincronización frame-perfecta: un input grabado en frame 5 puede moverse al frame 7. | La reproducción no coincide con la grabación original; desincronización garantizada. | Eliminar modificación de frames en `cleanInputs`. Solo filtrar duplicados y ordenar. |
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

**Causa principal confirmada por compilación (A-00):**

El cálculo de `frame` usaba `m_gameState.m_totalFrames`, un miembro que **no existe** en
los bindings de GD 2.2081. El mod no compilaba, por lo que nada de lo demás importaba:
mientras no compila, no hay macro que funcione. Se corrigió usando
`GJBaseGameLayer::m_currentStep`, el contador de pasos de física (240 Hz), que es
exactamente el mismo valor que GD usa internamente (`PlayerButtonCommand::m_step`) y que
otros mods de replay emplean.

**Causa secundaria confirmada por revisión de código (A-01/A-02):**

La función `cleanInputs()` en `src/replay.hpp` modificaba los valores `frame` de los inputs
grabados según `minHoldFrames` y `minGapFrames`, de modo que los frames guardados en el
`.gdr` no coincidían con los frames reales de captura. Además, el estado de deduplicación
de `shouldRecord()` eran variables `static` de archivo que persistían entre sesiones,
pudiendo descartar el primer input de una nueva grabación.

**Nota de honestidad:** estas dos causas secundarias son hallazgos de revisión de código; su
impacto exacto en juego está **pendiente de validación manual** en Android. No deben
presentarse como "confirmadas en juego".

---

## Estado de implementación

| Punto | Estado | Evidencia |
|---|---|---|
| A-00 `m_currentStep` | ✅ Hecho | Compila en CI (run `39007462340` success) |
| A-01 `cleanInputs` sin desplazar frames | ✅ Hecho | Revisión de código + CI |
| A-02 dedupe sin `static` | ✅ Hecho | Revisión de código + CI |
| A-04 eliminar `ignoreInput` | ✅ Hecho | Revisión de código + CI |
| A-05 `macrosDir()` con `getSaveDir()` | ✅ Hecho | Revisión de código + CI |
| A-06 clickbot sin offset | ✅ Hecho | Revisión de código + CI |
| A-07/A-08 clamp de speed/pitch | ✅ Hecho | Revisión de código + CI |
| A-09 `mod.json` Geode 5.11.2 | ✅ Hecho | CI con SDK nightly compila |
| A-10 null check en `renderStateSwitcher` | ✅ Hecho | Revisión de código + CI |
| A-11 logger bufferizado | ✅ Hecho | Revisión de código + CI |
| A-12 safe area bolita | ✅ Hecho | Revisión de código + CI |
| A-13 separar speed enabled/valor | ✅ Hecho | Constantes GMACRO_SPEED_*; UI y app coinciden |
| A-14 manejo offline updatecheck | ✅ Revisado | `!res.ok()` retorna sin crash (sin cambios) |
| A-15 código muerto `utils.hpp` | ✅ Hecho | Archivo eliminado |
| A-17 estado de botones sin dedupe | ✅ Hecho | `shouldRecord` siempre mantiene held |
| A-18 reset de fase del spam al pausar | ✅ Hecho | `spamWasOn=false` en el gate |
| A-19 hardening de `fromFile`/`save` | ✅ Hecho | try/catch + validación + CI |
| A-20 tests unitarios puros | ✅ Hecho | 14 casos, job de CI verde |
| A-21 workflow: release solo en main | ✅ Hecho | Build manual ya no publica |
| A-22 round-trip GDR real en tests | ✅ Hecho | `tests/test_gdr_roundtrip.cpp`, job de CI |
| A-16 `MACRO_DIAGNOSTIC.md` reescrito | ✅ Hecho | Archivo |
| A-03 semántica de cursor | ⚠️ Requiere test en juego | Documentado en test plan |

**Regla:** ✅ significa "compila y fue revisado", nunca "probado en juego".

---

## Plan de trabajo priorizado

### Fase 1 — P0: Arreglar macro end-to-end
1. ✅ Restaurar el build: `m_currentStep` en lugar de `m_totalFrames` (A-00).
2. ✅ Reescribir `shouldRecord` para eliminar estado `static` global (A-02).
3. ✅ Eliminar modificación de frames en `cleanInputs`; solo filtrar duplicados (A-01).
4. ✅ Null check en `renderStateSwitcher` antes de `createNewReplay` (A-10).
5. ✅ Corregir clickbot offset (A-06).
6. ✅ `macrosDir()` a `getSaveDir() / "macros"` (A-05).
7. ✅ tests unitarios (dedupe) + round-trip GDR (A-20/A-22).

### Fase 2 — P1: Estabilidad
1. ✅ Clamp de `speed` en speedhack (A-07).
2. ✅ Clamp del pitch de audio (A-08).
3. ✅ `ignoreInput` eliminado; `processCommands` original siempre se llama (A-04).
4. ✅ Logger: flush cada 10 líneas + `localtime_r` thread-safe (A-11).
5. ✅ `shouldRecord` mantiene el estado de botones aun sin dedupe (A-17).
6. ✅ Reset de fase del spam al pausar (A-18).
7. ✅ Hardening de `fromFile`/`save` (try/catch, validación) (A-19).

### Fase 3 — P2: UI móvil
1. ✅ Safe areas en bolita flotante (A-12).
2. ✅ Límites de speed/anti-spam consistentes entre UI y motor (A-13).
3. ✅ Mensajes de error claros en guardado/carga (A-19).
4. ✅ No borrar la grabación en curso al re-tocar Record.

### Fase 4 — P3/P4: SDK, CMake, CI, docs
1. ✅ `mod.json` a Geode 5.11.2 (A-09).
2. ✅ Workflow: release solo en push a `main` (A-21).
3. ✅ `docs/MACRO_TEST_PLAN.md` y `docs/RESEARCH_SOURCES.md` (A-16).
4. ✅ `MACRO_DIAGNOSTIC.md` reescrito con causa raíz real.
5. ✅ `CHANGELOG.md`, `README_ZBOT.md` y `about.md` actualizados.
6. ✅ Eliminar código muerto (`utils.hpp`) (A-15).

---

## Limitaciones del entorno actual

- No se dispone de Geometry Dash ni de Android NDK/Geode SDK local para compilar ni ejecutar pruebas en tiempo de ejecución.
- `GEODE_SDK` no está definido; `CMakeLists.txt` falla con `FATAL_ERROR` si no está definido.
- No se pueden ejecutar tests de juego real. Solo análisis estático y tests unitarios aislados de funciones puras.
- Todas las afirmaciones de "probado" se limitan a revisión de código y lógica; se requiere validación manual en Android por parte del usuario.
