# Credits and Acknowledgments

**G-Macro Android** is built upon the foundational work of the Geometry Dash modding
community. We would like to express our gratitude to the following projects and individuals.

---

## Framework and format

| Project | Author(s) | What we use |
|---|---|---|
| **Geode SDK** | Geode SDK Team | The modding framework that makes Android modding possible. |
| **GDReplayFormat** | maxnut | The standard **GDR** replay format (`gdr::Input`, `gdr::Replay`, msgpack/JSON) used for saving and loading macros. Consumed as a pinned git dependency, not copied. |

---

## Macro bots studied as reference

These projects were studied while designing the recording, playback, frame-sync,
checkpoint and dual-mode behaviour. No code was transplanted verbatim: the algorithms and
behavioural requirements were **re-implemented independently** in this codebase.

| Project | Author(s) | What we learned from it |
|---|---|---|
| **Eclipse Menu** | EclipseMenu team | GDR2 format usage, the "robust practice fix" playback approach, floating-icon mobile UI, per-platform macro storage. |
| **xdBot** | ZiLko | Classic bot behaviour: physics-state playback, clickbot, known desync/first-jump bugs worth avoiding. |
| **zBot** | FigmentBoy | Practice-mode fixes, frame advance, speedhack, record/playback manager structure. |
| **Silicate** | Silicate team | Frame counters and editor/play differences, trajectory recording, practice checkpoint handling. |
| **OpenHack / Zephyrus** | Prevter | Auto-scaling UI by framebuffer resolution (the basis of our mobile UI scaling). |
| **EclipseMenu-Modification** | Rustring | Recorder/replay structure and keybind handling of a maintained Eclipse fork. |
| **macro-tools** | gmd | The comparative catalog that maps which bot uses which playback method and macro format. |

---

## Notes on permission and licensing

Permission to use these source codes as reference was granted by their authors. In line
with their request, this project **credits every authorship explicitly** (this file and the
"Créditos" section of `about.md`).

Independently of that permission, this project deliberately:

* consumes **GDReplayFormat** as a pinned dependency rather than copying its code;
* **re-implements** techniques from scratch instead of transplanting source, so that each
  behaviour here is auditable and testable in our own `tests/`;
* does **not** vendor code from projects without a permissive licence.

Developed and maintained by **pepitogumball-lang**.
