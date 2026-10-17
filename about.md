# G-Macro Android

Una utilidad de **macros** para Geometry Dash en Android con grabación y reproducción
**frame-accurate**, usando el formato **GDR** (compatible con otros bots/replays).

## Características

- **Grabación y reproducción precisa**: los inputs se registran y reproducen contra el
  contador de pasos de física del juego (240 Hz).
- **Formato GDR**: compatibilidad con el formato estándar (msgpack binario y JSON).
- **Speedhack integrado**: cambia la velocidad del juego con sincronización de pitch de audio.
- **Clickbot**: sonido de click sincronizado con el frame exacto del input durante el playback.
- **Spam / autoclicker**: botón, jugador (P1/P2/ambos) y CPS configurables.
- **Frame Advance**: avanza el juego exactamente un frame (útil para TAS).
- **UI optimizada para móvil**: bolita flotante arrastrable + panel de pestañas.

## Uso rápido

1. Toca la **bolita Z** para abrir el panel.
2. En la pestaña **Home**, elige **Record**, entra al nivel y juega. Por defecto solo se
   guardan runs perfectos (nivel completado).
3. Para reproducir: carga un macro en la pestaña **Macro**, pulsa **Replay** y reinicia el nivel.
4. Ajusta velocidad en **Speed** y el autoclicker en **Spam**.

Los macros se guardan como `.gdr` en `Mod::get()->getSaveDir() / "macros"`.

## Estado

Compilado y verificado en CI para Android32 y Android64. La lógica pura está cubierta por
tests unitarios. El comportamiento en juego se valida manualmente (ver `docs/MACRO_TEST_PLAN.md`).

## Créditos

Mantenido por **pepitogumball-lang**. Gracias al equipo del Geode SDK y a los creadores
del formato GDR.
