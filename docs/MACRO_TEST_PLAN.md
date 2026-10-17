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
| M-09 | Speedhack | Activar speedhack 2x, grabar y reproducir | La velocidad cambia; la macro sigue sincronizada | Pendiente |
| M-10 | Archivo corrupto | Intentar cargar un `.gdr` truncado | Error claro, no crashea el juego | Pendiente |
| M-11 | Guardado al salir | Salir de nivel sin completar (`perfectRunOnly` off) | El macro se guarda al salir | Pendiente |
| M-12 | No grabar sin nivel | Activar RECORD en menú principal | No crashea; mensaje claro o simplemente no graba | Pendiente |
| M-13 | Clickbot sincronizado | Activar clickbot, reproducir macro | El sonido de click coincide con el salto visual | Pendiente |
| M-14 | Spam en macro | Activar spam, grabar con `spamRecordToMacro` on | Los eventos de spam se guardan en el macro | Pendiente |

## Criterios de aceptación P0

Solo se considera resuelto el bug de macro cuando se cumple TODO lo comprobable:

1. El número de inputs grabados, escritos, leídos e inyectados concuerda (salvo filtros explícitos documentados).
2. Presiones y releases se conservan y ejecutan correctamente, incluso con varios eventos en el mismo frame.
3. Los frames no retroceden inesperadamente ni se basan en unidades temporales incompatibles.
4. El primer frame, reinicios y checkpoints tienen semántica consistente.
5. Los errores de guardado/carga se detectan y no se presentan como éxito.
6. Existe al menos una prueba real en Android (checklist M-01 a M-14 ejecutada por el usuario).
