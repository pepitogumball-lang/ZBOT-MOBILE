# Cambios de G-Macro Android

### Pendiente de release (rama `opencode/zbot-mobile-improvements-2026-10-16`)

> **Estado:** compilado y testeado **automáticamente** en CI (Android32 + Android64 + tests
> unitarios). La validación **en juego real** sigue pendiente de un dispositivo Android
> (ver `docs/MACRO_TEST_PLAN.md`, casos M-15..M-25).

**Sincronización grabación/reproducción**
* **FIX (P0):** El `processCommands` original del motor ahora se ejecuta **antes** de
  inyectar los inputs del macro. Es el mismo orden que usan EclipseMenu, zBot y
  ToastyReplay: al leer `m_currentStep` en la misma fase que durante la grabación
  desaparece el clásico desfase de 1 frame entre grabar y reproducir.
* **Regla única de disparo:** la lógica del cursor de reproducción se extrajo a
  `zmacro::firstNotDue()` (`src/macro_utils.hpp`), usada por el bucle de playback, por
  el clickbot y por la re-alineación tras respawn/carga de macro. Un input con frame
  igual al frame actual se dispara **ya** (el "primer salto del macro" ya no se puede
  perder) y el cursor nunca retrocede.
* **FIX:** Al cargar un macro con el nivel ya empezado (`justLoaded`) o desde un
  checkpoint/StartPos, el cursor se alinea al frame actual en vez de quedarse en 0 y
  re-ejecutar todo.
* **Nuevo:** ajuste **Frame Offset** (−10..+10, persistente) para afinar la
  compatibilidad de frames con macros de otros bots (pestaña Speed).

**Robustez en marcha (guards)**
* **FIX:** No se inyectan inputs ni se spamea mientras el jugador está **muerto** o
  durante la **animación de fin de nivel** (evita inputs fantasma y botones pegados
  tras el respawn). El cursor se re-alinea al reaparecer.
* **FIX:** Tampoco se **graba** durante la muerte o el fin de nivel: esos eventos no son
  reproducibles y solo ensuciaban el `.gdr`.
* **FIX:** `fromFile` sanea el `framerate` de GDR ajenos (0, negativo, absurdo o
  irreal) para que no rompan duraciones ni cálculos derivados.

**Recuperación ante crashes (Android)**
* **Nuevo:** **copia de seguridad periódica** mientras se graba
  (`Auto guardar` + `Solo cada (s)`, por defecto 60 s, 0 = off). Se escribe en
  `<nivel>.autosave.gdr`, que es un `.gdr` normal visible en la lista de macros: si
  Android mata el juego a mitad de una grabación larga, el run se puede recuperar.
  **Nunca** sobreescribe el macro real y se borra al guardarse el run definitivo.
  El intervalo se mide con reloj monótono (segundos reales, no lo altera el speedhack).
* **Nuevo:** al borrar un macro se elimina también su `.autosave` huérfano.
* **MEJORA:** el log rota a los 512 KB (`zbot_debug.log` → `zbot_debug_prev.log`) para
  no llenar el almacenamiento del dispositivo.
* **MEJORA:** el log de `FIRE` en playback se limita a los primeros 24 inputs de cada
  epoch y luego 1 de cada 100: antes generaba megas de texto con macros de spam.

**UI móvil (Android-first)**
* **MEJORA:** El control principal (Home) pasa de radio-buttons pequeños a **tres
  botones a ancho completo y altura táctil** (IDLE / GRABAR / REPRODUCIR), con color
  del modo activo y aviso cuando no hay nivel o macro.
* **MEJORA:** El panel principal se dimensiona según la pantalla y se centra la primera
  vez (antes 460×600 fijos), con mínimos seguros para pantallas pequeñas y scroll
  automático si el contenido no cabe.
* **MEJORA:** La bolita flotante escala con la pantalla (26–64 px de radio) y su umbral
  de arrastre también: un toque ya no la mueve por el micro-movimiento del dedo.
* **MEJORA:** **Escalado automático de toda la interfaz** según la resolución real
  (1920 px de ancho = 1×), con tamaño de fuente, espaciados y áreas táctiles coherentes
  en cualquier móvil/tablet. Además hay un ajuste **Escala UI (0.5×–2×)** en *Settings*
  (botones −/+) para corregirlo en el propio dispositivo sin recompilar.
* **MEJORA:** Objetivos táctiles más grandes en Android (`TouchExtraPadding`, scrollbars
  y tiradores más gruesos), rejilla de presets de velocidad 4×2, botones +/- en Frame
  Offset y `Avanzar 1 frame` con altura táctil.

**Tests**
* **MEJORA:** 6 casos nuevos (T-14..T-19) cubren el cursor de playback, incluida la
  **regresión del primer salto** y la alineación tras respawn.

### v1.8.0
* **FIX BLOQUEANTE (build):** El mod **no compilaba** desde el commit `d402bf3`. Se usaba `GJGameState::m_totalFrames`, un miembro que **no existe** en los bindings de GD 2.2081. Se corrigió usando `GJBaseGameLayer::m_currentStep`. Sin este arreglo nada funcionaba: el mod ni siquiera cargaba. Verificado por compilación real en CI.
* **FIX CRÍTICO (P0):** `cleanInputs()` en `replay.hpp` ya no modifica los frames de los inputs grabados. Antes desplazaba `in.frame` según `minHoldFrames`/`minGapFrames`, rompiendo la sincronización frame-perfecta entre grabación y reproducción. Además, `save()` ahora respeta el ajuste **"Deduplicar inputs"** (antes deduplicaba siempre).
* **UI (P2):** Los ajustes **Min hold / Min gap** quedan marcados como **inactivos** (sin efecto), ya que desplazar frames es incompatible con la grabación frame-perfecta. Se conservan por compatibilidad de ajustes.
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
