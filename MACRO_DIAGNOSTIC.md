# Diagnóstico Técnico: Fallos de Estabilidad en Macros (G-Macro Android)

**Última actualización:** 17 oct 2026
**Commit auditado:** `d402bf3` (punto de partida) → rama de trabajo `opencode/zbot-mobile-improvements-2026-10-16`

> Este documento describe la causa raíz **real** del fallo de la macro, verificada
> por el log de compilación de CI y por revisión de código. El diagnóstico original
> (`m_currentProgress / 2`) está al final, archivado. **No** se ha re-aplicado.

---

## Causa raíz 0 (BLOQUEANTE): el mod no compilaba

El commit `d402bf3` (titulado *"Fix macro stability by using totalFrames"*) introdujo
`GJGameState::m_totalFrames`, un miembro que **no existe** en los bindings de GD 2.2081.
El compilador lo confirmó de forma explícita en el CI:

```
src/playbackmanager.cpp:40:46: error: no member named 'm_totalFrames' in 'GJGameState';
did you mean 'm_totalTime'?
src/recordmanager.cpp:54:52: error: no member named 'm_totalFrames' in 'GJGameState'
src/gui.cpp:301:55: error: no member named 'm_totalFrames' in 'GJGameState'
```

Como el mod no compilaba, **nada** funcionaba: no era un problema de sincronización
de macro, era que el `.geode` no se podía construir ni cargar. Cualquier análisis de
la macro sobre esa base era irrelevante.

**Miembro real:** `GJGameState` tiene `double m_totalTime`, `unsigned int m_currentProgress`
y `unsigned int m_commandIndex`, pero **no** `m_totalFrames`.

**Corrección intermedia (ya superada):** todas las referencias a `m_gameState.m_totalFrames`
(~4) se sustituyeron por `GJBaseGameLayer::m_currentStep`. Eso arregló la compilación
(verificado en CI, Android32 + Android64), pero **no** arregló el macro.

### Corrección definitiva: el contador es `m_gameState.m_currentProgress`

La primera prueba en un dispositivo real (Galaxy Tab A8, GD 2.2081, 60 Hz) demostró que
`m_currentStep` **no sirve**:

```
[PLAYBACK] tick frame=0 cursor=0/4 remaining=4     (repetido cientos de veces)
[PLAYBACK] el frame del juego no avanza | frame=0 | currIndex=0
```

El hook llama al `processCommands` original a ~60 Hz, pero `m_currentStep` se quedaba
**clavado en 0**. El cursor (`firstNotDue`) nunca encontraba inputs vencidos, así que el
macro no disparaba absolutamente nada. Encaja con su posición en los bindings: `m_currentStep`
vive en `GJBaseGameLayer` junto a la maquinaria de replay/TAS integrada del propio GD
(`m_queuedButtons`, `m_queuedReplayButtons`, `RecordCheckpoint`, `m_replayRandSeed`),
no es el contador de juego normal.

**Se cambió a `GJGameState::m_currentProgress`**, que es el contador que usan **todos** los
bots de referencia estudiados (Eclipse `Bot.cpp`, zBot, xdBot, Silicate) y que además es el
estado que `CheckpointObject` guarda y restaura. Aplicado en grabación, reproducción,
realineado tras respawn y en la GUI.

---

## Causa raíz 1 (P0): `cleanInputs()` modificaba los frames grabados

En `src/replay.hpp`, `cleanInputs()` desplazaba `in.frame` según `minHoldFrames`/`minGapFrames`.
Los frames guardados en el `.gdr` dejaban de coincidir con los frames reales de captura,
de modo que el playback disparaba inputs en momentos distintos a los grabados.

**Corrección aplicada:** se eliminó el desplazamiento. Ahora solo ordena por frame y
descarta eventos redundantes (no-ops). La lógica se extrajo a `src/macro_utils.hpp`
(`dedupeNoOps`, `sortByFrame`) y está cubierta por tests unitarios.

## Causa raíz 2 (P0): estado de deduplicación en variables `static`

En `shouldRecord()` las variables `s_lastFrame`/`s_lastDown`/`s_lastButton`/`s_lastP2`
eran `static` de archivo y persistían entre sesiones de grabación.

**Corrección aplicada:** el estado pasó a miembros de `GMacro`
(`dedupeLastFrame`, `dedupeLastDown`, `dedupeLastButton`, `dedupeLastP2`) y se resetea
con `resetDedupeState(mgr)`. Además, `shouldRecord` ahora mantiene **siempre** el estado
`p1ButtonHeld`/`p2ButtonHeld` (aunque la deduplicación esté desactivada), para no perder
los "release" al reiniciar el nivel.

## Causa raíz 3 (P0): clickbot con offset

El clickbot reproducía el sonido con un offset de ~100 ms respecto al input.

**Corrección aplicada:** el sonido se reproduce en el mismo frame del input.

## Causa raíz 4 (P0): ruta de macros hardcodeada en Android

`macrosDir()` usaba una ruta absoluta bajo `/storage/emulated/0/...`, problemática con
scoped storage en Android 10+.

**Corrección aplicada:** `macrosDir()` usa `Mod::get()->getSaveDir() / "macros"`.

## Causa raíz 5 (P1): cursor de playback atascado

Si el frame del juego no avanza (pausa o frame advance sin avance), el cursor de playback
podía quedar detenido.

**Corrección aplicada (revisada):** se dejó como **diagnóstico de solo lectura**: se
registra una advertencia en el log tras 120 ciclos sin avance, pero **no** se resetea el
cursor automáticamente. Resetearlo re-ejecutaría inputs ya pasados y empeoraría la
desincronización. (La versión intermedia que sí lo reseteaba fue descartada.)

---

## Nota de honestidad

Todo lo anterior está respaldado por **compilación real en CI** y por **tests unitarios**.
El comportamiento **en juego** (si grabar y reproducir observan `m_currentStep` en la
misma fase, cómo se comporta con checkpoints y reinicios, etc.) **no está validado en un
dispositivo Android**. Ese es el siguiente paso obligatorio: ver `docs/MACRO_TEST_PLAN.md`.

---

## Diagnóstico original (OBSOLETO — archivado por referencia)

En `recordmanager.cpp` y `playbackmanager.cpp`, el frame se calculaba así:

```cpp
int frame = static_cast<int>(this->m_gameState.m_currentProgress) / 2;
```

`m_currentProgress` no es un contador de frames fiable para macros de alta precisión en
2.2081, y dividir entre 2 es una asunción arbitraria. Este diagnóstico **nunca se confirmó**
como causa raíz y **no debe volver a aplicarse**. El campo nunca fue la causa bloqueante;
el verdadero problema era que el mod no compilaba (ver Causa raíz 0).
