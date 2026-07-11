# Diagnóstico Técnico: Fallos de Estabilidad en Macros (G-Macro Android)

Tras analizar los logs de consola proporcionados y el código fuente del mod, se han identificado varios problemas críticos que explican por qué los macros son inestables ("una mierda").

## 1. Problema de Precisión de Frames (Causa Raíz)
En `recordmanager.cpp` y `playbackmanager.cpp`, el frame se calcula así:
```cpp
int frame = static_cast<int>(this->m_gameState.m_currentProgress) / 2;
```
**El problema:** `m_currentProgress` no es un contador de frames fiable para macros de alta precisión en la versión 2.2081. Además, dividir por 2 es una asunción arbitraria que causa desincronización inmediata si el framerate del juego no coincide exactamente con lo esperado.

## 2. Anomalías en los Logs (Retroceso de Frames)
Los logs muestran que durante el `RECORD`, los frames a veces retroceden o se repiten de forma inconsistente:
*   Línea 168: `frame=10277`
*   Línea 169: `frame=10214` (¡Retroceso de 63 frames!)
*   Línea 170: `frame=10150` (¡Otro retroceso!)

Esto indica que `m_currentProgress` fluctúa o se ve afectado por el motor de física de forma no lineal durante la grabación, lo que corrompe el macro.

## 3. Fallo en el Playback (Bucle Infinito)
En el log de `PLAYBACK`:
```
20:26:33 [PLAYBACK] tick frame=0 cursor=0/116 remaining=116
...
20:26:35 [PLAYBACK] tick frame=0 cursor=0/116 remaining=116
```
El cursor **nunca avanza** (`cursor=0/116`). Esto sucede porque `inputs[m_fields->currIndex].frame <= frame` nunca se cumple o el `frame` calculado en el playback se queda estancado en 0 o valores muy bajos debido a la inconsistencia de `m_currentProgress`.

## 4. Plan de Acción Técnico
1.  **Cambiar Fuente de Frames:** Utilizar un contador de frames basado en el motor de física (`m_gameState.m_totalFrames` o similar) en lugar de `m_currentProgress`.
2.  **Sincronización Determinista:** Asegurar que el `frame` usado en `RECORD` sea idéntico al de `PLAYBACK`.
3.  **Eliminar "Magia Negra":** Reemplazar la división `/ 2` por una gestión de tiempo real o frames totales reales.
4.  **Limpieza de Logs:** Reducir el ruido en la consola para mejorar el rendimiento en Android.
