# Fuentes de Investigación — G-Macro Android

**Fecha de consulta:** 16 de octubre de 2026  
**Propósito:** Documentar fuentes externas estudiadas para informar decisiones de diseño y corrección.

---

## A. Geode: documentación y SDK

| Fuente | URL | Fecha consultada | Versión/revisión | Qué se obtuvo |
|---|---|---|---|---|
| Geode docs — Hooking / Modifying classes | https://docs.geode-sdk.org/tutorials/modify/ | 2026-10-16 | Actual | Sintaxis `$modify`, llamadas al original, ciclo de vida, modificadores de clase |
| Geode docs — Configuring mods (mod.json) | https://docs.geode-sdk.org/mods/configuring/ | 2026-10-16 | Actual | Campos requeridos de mod.json, `geode`, `gd` por plataforma, versionado semántico |
| Geode docs — Saving data | https://docs.geode-sdk.org/mods/savedata/ | 2026-10-16 | Actual | `setSavedValue`/`getSavedValue`, `Mod::get()->getSaveDir()`, directorios por plataforma |
| Geode releases | https://github.com/geode-sdk/geode/releases | 2026-10-16 | v5.11.2 (15 oct 2026) | Release estable más reciente; corrige crashes en GD 2.209, dirigida principalmente a 2.2081 |
| Geode create-mod guide | https://github.com/geode-sdk/docs/blob/main/getting-started/create-mod.md | 2026-10-16 | Actual | Estructura estándar: `CMakeLists.txt`, `mod.json`, `src/`, `geode build`, Android32/Android64 |
| Geode publishing guidelines | https://docs.geode-sdk.org/mods/guidelines/ | 2026-10-16 | Actual | Reglas del Index, no reemplazar releases, código fuente disponible |
| Geode publishing | https://docs.geode-sdk.org/mods/publishing/ | 2026-10-16 | Actual | Proceso de publicación, assets, versionado |

**Decisión (APLICADA):** El mod.json declaraba Geode 5.8.0; ya se actualizó a **5.11.2** y se verificó por compilación real en CI. No cambiar a 2.209 sin verificar bindings.

---

## B. GDReplayFormat (formato GDR)

| Fuente | URL | Fecha consultada | Versión/revisión | Qué se obtuvo |
|---|---|---|---|---|
| GDReplayFormat repo | https://github.com/maxnut/GDReplayFormat | 2026-10-16 | commit `9c32b55` (14 nov 2024) | API de `gdr::Input`, `gdr::Replay`, `importData`, `exportData`, msgpack/JSON, extensiones |
| GDReplayFormat README | https://github.com/maxnut/GDReplayFormat | 2026-10-16 | commit `9c32b55` | Documentación de formato: `.gdr` = msgpack, `.gdr.json` = JSON, `frame` = tiempo × framerate, `button` = PlayerButton, `player2`, `down` |
| GDReplayFormat commit verificado | https://github.com/maxnut/GDReplayFormat/commits/9c32b5529067901418ea6e4dfbd36437dc8712bc | 2026-10-16 | `9c32b55` | Commit firmado, fecha 14 nov 2024, merge de PR #2 |

**Decisión:** Mantener la revisión fijada `9c32b55`. La API es estable y documentada. `Input.frame` es `uint32_t`; `Replay.framerate` es `float` (240 por defecto). No actualizar automáticamente.

---

## C. zBot original (referencia de comportamiento, NO copia de código)

| Fuente | URL | Fecha consultada | Licencia | Qué se obtuvo |
|---|---|---|---|---|
| zBot repo | https://github.com/FigmentBoy/zBot | 2026-10-16 | EULA propietaria | README describe grabación/reproducción precisa, practice fixes, speedhack, frame advance |
| zBot LICENSE | https://github.com/FigmentBoy/zBot/blob/main/LICENSE | 2026-10-16 | EULA | Prohíbe distribuir copias, restringe reverse engineering. Permite compilar/modificar para uso personal o proponer contribuciones |

**Límite de licencia:** No se copia ni se trasplanta código de zBot. Solo se usa como referencia de comportamiento y requisitos funcionales.

---

## D. EclipseMenu y mods relacionados

| Fuente | URL | Fecha consultada | Estado | Qué se obtuvo |
|---|---|---|---|---|
| Eclipse Menu web | https://eclipse.menu/ | 2026-10-16 | Anuncia v1.9.4 | Acceso desde icono flotante en móvil, guardado de macros por plataforma |
| EclipseMenu repo (404) | https://github.com/EclipseMenu/EclipseMenu | 2026-10-16 | 404 / no disponible | No se puede verificar como repositorio canónico |
| EclipseMenu-Modification | https://github.com/Rustring/EclipseMenu-Modification | 2026-10-16 | EPL-2.0 | Recorder interno, replay bot, keybinds, personalización (proyecto relacionado, no canónico) |
| EclipseGDMainMacros | https://github.com/Sovenok-Hacker/EclipseGDMainMacros | 2026-10-16 | Público | Archivos `.gdr2`, referencia de formato/organización de macros |
| macro-tools index | https://codeberg.org/gmd/macro-tools | 2026-10-16 | No mantenido | Índice comparativo de bots/macro/TAS (no usar como garantía) |
| globed2 | https://github.com/GlobedGD/globed2 | 2026-10-16 | Público | Referencia de organización multiplataforma, CI, compatibilidad Geode |

**Decisión:** Extraer principios (UI táctil, icono flotante, gestión de archivos de macros), no copiar código.

---

## E. Proyecto ZBOT-MOBILE (objeto de auditoría)

| Fuente | URL | Fecha consultada | Qué se obtuvo |
|---|---|---|---|
| Repositorio principal | https://github.com/pepitogumball-lang/ZBOT-MOBILE | 2026-10-16 | Rama `main`, 22 commits. *(Estado al 17 oct: existe un release `v1.8.0`; ver `docs/PROJECT_AUDIT.md`.)* |
| mod.json público | https://github.com/pepitogumball-lang/ZBOT-MOBILE/blob/main/mod.json | 2026-10-16 | Geode 5.8.0, GD 2.2081, versión v1.7.0 |
| README_ZBOT.md | https://github.com/pepitogumball-lang/ZBOT-MOBILE/blob/main/README_ZBOT.md | 2026-10-16 | Afirma macros precisas, GDR, speedhack, clickbot de audio, UI móvil |
| MACRO_DIAGNOSTIC.md | https://github.com/pepitogumball-lang/ZBOT-MOBILE/blob/main/MACRO_DIAGNOSTIC.md | 2026-10-16 | Acusa `m_currentProgress / 2` (línea ya no existe en el código actual) |
| CHANGELOG.md | https://github.com/pepitogumball-lang/ZBOT-MOBILE/blob/main/CHANGELOG.md | 2026-10-16 | v1.7.0 a v1.5.0 con notas de fixes previos |

---

## Conclusiones de la investigación

1. **GDReplayFormat** es el formato interoperable estándar; el proyecto ya lo usa correctamente en estructura, pero `cleanInputs()` rompe la fidelidad de frames.
2. **Geode v5.11.2** es la release estable; el mod declara 5.8.0 y debe actualizarse tras verificar compilación.
3. **zBot** es referencia de comportamiento únicamente; su licencia prohíbe copia de código.
4. **EclipseMenu** ofrece principios de UI móvil (icono flotante) pero su repositorio canónico no está verificable (404).
5. No se debe actualizar dependencias CPM automáticamente; solo en cambios identificables y comprobables.
