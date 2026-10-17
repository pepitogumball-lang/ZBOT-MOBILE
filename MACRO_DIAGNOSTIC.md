# Diagnóstico Técnico: Fallos de Estabilidad en Macros (G-Macro Android)

**Estado: OBSOLETO — el diagnóstico anterior a `d402bf3` está abajo, archivado. La causa raíz real se describe más abajo.**

---

## Diagnóstico actual (16 oct 2026, commit `d402bf3`)

El bug original (`m_currentProgress / 2`) fue corregido en un commit anterior que reemplazó `m_currentProgress` por `m_gameState.m_totalFrames`. Sin embargo, la macro seguía sin funcionar correctamente. La causa raíz real identificada durante la auditoría de código es:

### Causa raíz 1 (P0): `cleanInputs()` modifica los frames de los inputs grabados

En `src/replay.hpp`, `cleanInputs()` desplazaba `in.frame` según `minHoldFrames`/`minGapFrames`. Esto significa que los frames almacenados en el archivo `.gdr` **no coinciden** con los frames reales en los que se capturaron los inputs. Cuando el playback compara `inputs[currIndex].frame <= m_total_frames`, los frames desplazados causan desincronización.

**Corrección aplicada:** La sección de `cleanInputs()` que modificaba frames fue eliminada. Ahora solo filtra duplicados y ordena por frame.

### Causa raíz 2 (P0): Variables `static` en `shouldRecord()` persisten entre sesiones

En `src/recordmanager.cpp`, las variables `static` `s_lastFrame`, `s_lastDown`, `s_lastButton`, `s_lastP2` nunca se reseteaban correctamente entre sesiones de grabación. Solo se reseteaban en `PlayLayer::init` cuando `state == RECORD`, pero no al cambiar de nivel o detener/reiniciar grabación desde la UI.

**Corrección aplicada:** El estado de deduplicación se movió a miembros de `GMacro` (`dedupeLastFrame`, `dedupeLastDown`, `dedupeLastButton`, `dedupeLastP2`) y se resetea explícitamente con `resetDedupeState(mgr)`.

### Causa raíz 3 (P0): Clickbot con offset de 100 ms

En `src/playbackmanager.cpp`, el clickbot reproducía sonidos con `frame + framerate * 0.1` (100 ms antes del input real), desincronizando el audio de la acción visual.

**Corrección aplicada:** El offset se eliminó; el sonido se reproduce en el mismo frame del input.

### Causa raíz 4 (P0): Ruta hardcodeada de macros en Android

En `src/replay.hpp`, `macrosDir()` en Android usaba `/storage/emulated/0/Android/media/com.geode.launcher/game/geode/macros`, que puede no ser accesible con scoped storage en Android 10+.

**Corrección aplicada:** `macrosDir()` ahora usa `Mod::get()->getSaveDir() / "macros"` en todas las plataformas.

### Causa raíz 5 (P0): Cursor de playback atascado

En `src/playbackmanager.cpp`, si `m_totalFrames` no avanza (frame advance sin `doAdvance`, juego pausado), el cursor se quedaba en 0 para siempre. Se añadió una guardia que detecta 120 frames sin progreso y resetea el cursor.

---

## Diagnóstico original (obsoleto, commit anterior a `d402bf3`)

En `recordmanager.cpp` y `playbackmanager.cpp`, el frame se calculaba así:
```cpp
int frame = static_cast<int>(this->m_gameState.m_currentProgress) / 2;
```
`m_currentProgress` no es un contador de frames fiable para macros de alta precisión en la versión 2.2081. Dividir por 2 es una asunción arbitraria que causa desincronización inmediata si el framerate del juego no coincide exactamente con lo esperado.

**Estado:** Ya corregido en el commit `d402bf3` ("Refactor: Fix macro stability by using totalFrames").

---

## Recomendación

Este archivo se archiva para referencia histórica. Para diagnóstico actual, consultar `docs/PROJECT_AUDIT.md` y `docs/MACRO_TEST_PLAN.md`.
