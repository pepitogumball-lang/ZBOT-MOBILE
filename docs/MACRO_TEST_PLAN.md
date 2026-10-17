# Plan de Pruebas — Macro End-to-End (G-Macro Android)

**Fecha:** 16 de octubre de 2026  
**Estado:** Pendiente de validación en dispositivo real  
**Limitación:** No se puede ejecutar Geometry Dash en este entorno. Las pruebas automatizadas cubren funciones puras (deduplicación, serialización, validación). Las pruebas de juego real requieren validación manual en Android.

---

## Pruebas automatizadas (funciones puras)

Estas pruebas pueden ejecutarse sin Geometry Dash, compilar código aislado de `replay.hpp` y `recordmanager.cpp`:

| ID | Caso | Entrada | Resultado esperado | Estado |
|---|---|---|---|---|
| T-01 | Macro vacía | `inputs = {}` | `save()` debe rechazar o generar archivo vacío válido; `fromFile()` no debe crashear | Pendiente |
| T-02 | Un solo click (press/release) | `addInput(10, 1, false, true)`, `addInput(15, 1, false, false)` | Round-trip: `importData(exportData())` conserva ambos eventos con frames exactos | Pendiente |
| T-03 | Varias pulsaciones rápidas | Inputs en frames 10, 12, 14, 16 (misma tecla) | Todos los eventos conservados; no se descarta ninguno por deduplicación | Pendiente |
| T-04 | Hold largo | `addInput(10, 1, false, true)`, `addInput(100, 1, false, false)` | Press en frame 10, release en frame 100; `cleanInputs` no desplaza frames | Pendiente |
| T-05 | Múltiples eventos mismo frame | `addInput(50, 1, false, true)`, `addInput(50, 2, false, true)` | Ambos eventos conservados en frame 50 | Pendiente |
| T-06 | Input frame 0 | `addInput(0, 1, false, true)` | Frame 0 válido; playback debe ejecutarlo en el primer frame | Pendiente |
| T-07 | Macro larga | 1000+ inputs con frames crecientes | Round-trip sin pérdida; orden preservado | Pendiente |
| T-08 | Archivo truncado | Bytes parciales de un `.gdr` válido | `fromFile()` debe retornar nullptr o error explícito, no crashear | Pendiente |
| T-09 | Frames no monotónicos | Inputs con frames 100, 50, 75 | `cleanInputs` ordena por frame; no hay retrocesos | Pendiente |
| T-10 | Botón fuera de rango | `addInput(10, 99, false, true)` | `cleanInputs` conserva el evento pero marca para validación | Pendiente |
| T-11 | `minHoldFrames`/`minGapFrames` | Valores > 0 aplicados | **NUNCA** modificar `in.frame`; solo filtrar duplicados | Pendiente |
| T-12 | Deduplicación | Dos eventos idénticos (mismo frame, btn, p2, down) | Solo se conserva uno | Pendiente |
| T-13 | `purgeAfter(frame)` | Inputs con frames >= frame eliminados | Inputs anteriores se conservan; posteriores se eliminan | Pendiente |

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
