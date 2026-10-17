# Plan de Pruebas — Macro End-to-End (G-Macro Android)

**Fecha:** 16 de octubre de 2026  
**Estado:** Pendiente de validación en dispositivo real  
**Limitación:** No se puede ejecutar Geometry Dash en este entorno. Las pruebas automatizadas cubren funciones puras (deduplicación, serialización, validación). Las pruebas de juego real requieren validación manual en Android.

---

## Pruebas automatizadas (funciones puras)

Desde v1.8.0 la lógica pura está en `src/macro_utils.hpp` (sin dependencias de Geode)
y se prueba en `tests/test_macro_utils.cpp`. El round-trip de serialización se prueba
contra la librería real GDReplayFormat en `tests/test_gdr_roundtrip.cpp`. Se ejecutan localmente con:

```sh
g++ -std=c++17 -Wall -Wextra -I src tests/test_macro_utils.cpp -o /tmp/test_macro && /tmp/test_macro

git clone https://github.com/maxnut/GDReplayFormat /tmp/gdr
git -C /tmp/gdr checkout 9c32b5529067901418ea6e4dfbd36437dc8712bc
g++ -std=c++17 -Wall -Wextra -I /tmp/gdr/include tests/test_gdr_roundtrip.cpp -o /tmp/test_gdr && /tmp/test_gdr
```

y en CI en el job **Unit tests**. La columna "Estado" distingue **AUTO** (cubierto por
esos tests), **CÓDIGO** (verificado por revisión de código, sin test aislado) y
**PENDIENTE** (sin cobertura todavía).

| ID | Caso | Entrada | Resultado esperado | Estado |
|---|---|---|---|---|
| T-01 | Macro vacía | `inputs = {}` | No crash; utilidades no rompen con lista vacía | AUTO (vacío) |
| T-02 | Un solo click (press/release) | frames 10 y 15 | Ambos eventos con frames exactos | AUTO (+ round-trip) |
| T-03 | Varias pulsaciones rápidas | frames 10,12,14,16 | Todos conservados | AUTO |
| T-04 | Hold largo | press 10, release 100 | Frames no se desplazan | AUTO |
| T-05 | Múltiples eventos mismo frame | frame 50, botones 1 y 2 | Ambos conservados | AUTO |
| T-06 | Input frame 0 | frame 0 | Frame 0 válido | AUTO |
| T-07 | Macro larga | 1000+ inputs | Sin pérdida; orden preservado | AUTO (round-trip 1500) |
| T-08 | Archivo truncado | bytes parciales | `fromFile()` retorna nullptr, no crashea | AUTO (lanza y se captura) |
| T-09 | Frames no monotónicos | 100, 50, 75 | Se ordenan; sin retrocesos | AUTO |
| T-10 | Botón fuera de rango | botón 99 | Se conserva sin corromper dedupe | AUTO |
| T-11 | `minHoldFrames`/`minGapFrames` | valores > 0 | **NUNCA** modifican `in.frame` | AUTO |
| T-12 | Deduplicación | eventos idénticos repetidos | Solo se conserva uno | AUTO |
| T-13 | `purgeAfter(frame)` | frames >= frame | Anteriores se conservan, posteriores se eliminan | AUTO |
| T-14 | Cursor en lista vacía | `firstNotDue({}, 0, 0)` | Devuelve 0 | AUTO |
| T-15 | Input en frame 0 | input frame 0, cursor en frame 0 | Está "vencido": el primer click nunca se salta | AUTO |
| T-16 | Borde exacto del cursor | input frame 5, cursor frame 4/5/6 | 4 = aún no; 5 y 6 = sí | AUTO |
| T-17 | Progresión de frames | 40 frames sobre 4 inputs | Cada input se dispara exactamente 1 vez; el cursor nunca retrocede | AUTO |
| T-18 | Alineación tras respawn | respawn en frame 10 | No re-ejecuta nada anterior; dispara lo justo del frame 10 | AUTO |
| T-19 | Cursor ya avanzado | `firstNotDue(v, 1, 50)` | No revisa elementos anteriores al cursor | AUTO |

## Pruebas manuales en Android (checklist para el usuario)

| ID | Caso | Cómo probar | Resultado de éxito | Estado |
|---|---|---|---|---|
| M-01 | Grabar un click | Activar RECORD, entrar a nivel, hacer un click, salir | Notification "Macro perfecto guardado" o archivo creado en `macros/` | Pendiente |
| M-02 | Reproducir macro | Cargar macro, activar PLAYBACK, reiniciar nivel | Los clicks se reproducen en los mismos frames | Pendiente |
| M-03 | Segundo playback sin reset | Reproducir mismo macro dos veces seguidas | El cursor no se queda en 0; segundo playback funciona igual | Pendiente |
| M-04 | Reset en practice | Grabar, resetear nivel, volver a grabar | No se pierde el primer click tras el reset | Pendiente |
| M-05 | Checkpoint | Grabar con checkpoint, continuar desde checkpoint | El playback salta a los inputs correctos desde el checkpoint | Pendiente |
| M-06 | Dual mode | Grabar en nivel dual (P1 y P2), reproducir | P1 y P2 reciben sus inputs respectivos | Pendiente |
| M-07 | Frame advance | Activar frame advance, avanzar un frame | El juego avanza exactamente un frame; no se congela | Pendiente |
| M-08 | Pausar y reanudar | Pausar durante playback, reanudar | El playback continúa desde el frame correcto | Pendiente |
| M-09 | Speedhack 2x | Activar speedhack 2x, grabar y reproducir a 2x | La velocidad cambia; la macro sigue sincronizada | Pendiente |
| M-10 | Archivo corrupto | Intentar cargar un `.gdr` truncado | Error claro, no crashea el juego | Pendiente |
| M-11 | Guardado al salir | Salir de nivel sin completar (`perfectRunOnly` off) | El macro se guarda al salir | Pendiente |
| M-12 | No grabar sin nivel | Activar RECORD en menú principal | No crashea; mensaje claro o simplemente no graba | Pendiente |
| M-13 | Clickbot sincronizado | Activar clickbot, reproducir macro | El sonido de click coincide con el salto visual | Pendiente |
| M-14 | Spam en macro | Activar spam, grabar con `spamRecordToMacro` on | Los eventos de spam se guardan en el macro | Pendiente |
| M-15 | Guardado de seguridad | Activar "Auto guardar" + intervalo 15 s, grabar 30 s y matar el juego desde el sistema | Aparece `<nivel>.autosave` en la lista de macros y se puede reproducir | Pendiente |
| M-16 | La copia no pisa el macro bueno | Tener un macro perfecto guardado, grabar otro intento y morir antes de completar | El `.gdr` bueno sigue intacto; solo existe/crece el `.autosave` | Pendiente |
| M-17 | Limpieza de la copia | Completar un nivel con "Auto guardar" on | El `<nivel>.autosave` desaparece al guardarse el run real | Pendiente |
| M-18 | Muerte durante playback | Reproducir un macro y morir a propósito | No quedan botones "pegados" al reaparecer; el playback se re-alinea | Pendiente |
| M-19 | UI táctil (Home) | Usar los 3 botones grandes IDLE/GRABAR/REPRODUCIR | Se pulsan con el dedo sin tocar otro control, y marcan el estado activo con color | Pendiente |
| M-20 | Panel adaptable | Abrir el panel en el móvil (vertical y horizontal) | El panel cabe en la pantalla, queda centrado y hace scroll si no cabe | Pendiente |
| M-21 | Bolita flotante táctil | Tocar la bolita (sin arrastrar) y luego arrastrarla | Un toque abre/cierra el panel sin moverla; arrastrar la mueve y guarda la posición | Pendiente |
| M-22 | Tooltips/ayuda en táctil | Mantener pulsado sobre un ajuste | El tooltip de ayuda se ve aun sin ratón (o el texto está visible) | Pendiente |
| M-23 | Log acotado | Grabar un macro largo con spam y revisar `zbot_debug.log` | El archivo no crece sin control: al pasar 512 KB rota a `zbot_debug_prev.log` | Pendiente |
| M-24 | Orden de hooks en reproducción | Reproducir y comparar el primer frame exacto de cada input | Los inputs caen en el mismo frame que al grabar (sin desfase de 1 frame) | Pendiente |
| M-25 | Escala de UI ajustable | Tocar los botones "−"/"+" de "Escala UI" en Settings y luego "Restablecer" | La interfaz cambia de tamaño al momento y el valor se conserva entre sesiones | Pendiente |
| M-26 | Divisoria de contadores (B-01) | Grabar unos clicks, poner un checkpoint, morir y reaparecer, y comparar en `zbot_debug.log` los campos `step=` y `prog=` de los logs `RECORD`/`EPOCH`/`FIRE` | `prog=` avanza y `step=` se queda en 0 (medido en dispositivo). Se cambió todo el mod a `m_currentProgress` | ✅ Resuelto (medido en dispositivo) |
| M-27 | **Grabar a 0.25x y reproducir a 1x** (el bug reportado) | Activar speedhack **0.25x**, grabar un run corto con varios clicks, guardar, quitar el speedhack (1x), cargar el macro y reproducir | El run se reproduce sincronizado a 1x: los inputs caen en las **mismas posiciones del nivel** que al grabar; no se agolpan al principio | ⏳ Pendiente (prueba en juego real) |
| M-28 | Contraste de índice por tiempo | En `zbot_debug.log`, tras M-27, comparar el campo `lt=` (tiempo de juego) y `frame=` nuevo con el viejo `prog=`: `frame` debe ser ≈ `prog / 2` y `lt` debe crecer de forma estable independientemente de la velocidad | `frame ≈ prog/2`; `lt` es coherente con el tiempo del nivel | ⏳ Pendiente (prueba en juego real) |

### Cómo leer el diagnóstico B-01 (`step=` vs `prog=`) — RESUELTO

**Resultado medido en el dispositivo real (Galaxy Tab A8, GD 2.2081):**

```
[PLAYBACK] tick frame=0 cursor=0/4 remaining=4     (repetido cientos de veces)
[PLAYBACK] el frame del juego no avanza | frame=0 | currIndex=0
```

`m_currentStep` se quedaba **clavado en 0** aunque el hook ejecutaba el `processCommands`
original a ~60 Hz. Con `frame=0` para siempre, `firstNotDue` no encontraba ningún input
vencido ⇒ el cursor no avanzaba ⇒ **el macro no disparaba nada**. Ese era el síntoma
reportado ("el macro no funciona").

También se descartó el frame advance como causa: el early-return está *encima* del log
`tick`, así que si estuviera activo no se habría impreso ni una sola línea `tick`.

**Corrección aplicada:** el índice de reproducción pasó a `GJGameState::m_currentProgress`
(el estado que `CheckpointObject` restaura), aplicado en `recordmanager.cpp` (grabación y
`purgeAfter`), `playbackmanager.cpp` (reproducción y realineado tras respawn) y `gui.cpp`.
Eso hizo que el macro ya grabara y reprodujera **a velocidad normal**.

**Superado a su vez por el indexado por tiempo de juego (M-27/M-28):** `m_currentProgress`
seguía siendo un contador de pasos de física, no de tiempo, y desincronizaba al grabar con
speedhack. Ahora el índice es `m_levelTime × framerate` (`GMacro::getFrame()`). En los logs,
`frame=` es el índice nuevo, `lt=` el tiempo de juego, y `step=`/`prog=` quedan solo como
contraste de diagnóstico: se espera que sigan avanzando pero que `frame ≈ prog / 2`.

## Criterios de aceptación P0

Solo se considera resuelto el bug de macro cuando se cumple TODO lo comprobable:

1. El número de inputs grabados, escritos, leídos e inyectados concuerda (salvo filtros explícitos documentados).
2. Presiones y releases se conservan y ejecutan correctamente, incluso con varios eventos en el mismo frame.
3. Los frames no retroceden inesperadamente ni se basan en unidades temporales incompatibles.
4. El primer frame, reinicios y checkpoints tienen semántica consistente.
5. Los errores de guardado/carga se detectan y no se presentan como éxito.
6. Existe al menos una prueba real en Android (checklist M-01 a M-14 ejecutada por el usuario).
