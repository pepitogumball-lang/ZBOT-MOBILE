# Credits

ZBOT-MOBILE is **not an original work**. It is a mobile-focused port and
extension of existing Geometry Dash macro / replay tools built within
the Geode ecosystem. This project reuses ideas, structure, and in some
cases adapted code from prior community work. Full attribution below.

## Original work

- **zBot** — original macro / replay system implementation  
  Primary upstream reference: https://github.com/FigmentBoy/zBot  

The core concepts used in this project — including recording,
playback flow, replay structure (`zReplay`), and state handling
(`zState`) — are derived from this implementation and related
community iterations.

ZBOT-MOBILE would not exist without these prior works.

---

## Additional references / inspirations

The following projects influenced specific systems and behaviors:

- **ReplayBot (matcool)**  
  Playback timing model, restart semantics, and frame-based triggering.

- **zBot variants (community forks)**  
  Cross-reference for consistency in replay handling and structure.

- **EclipseMenu**  
  UI style inspiration (dark theme, tab layout, macro list structure).

- **xdBot / EclipseMenu**  
  Speedhack design (delta-time scaling and optional audio pitch sync).

---

## Libraries

Used via CPM in `CMakeLists.txt`:

- **Geode SDK**  
  https://github.com/geode-sdk/geode  

- **GDReplayFormat** (maxnut)  
  https://github.com/maxnut/GDReplayFormat  

- **gd-imgui-cocos** (matcool)  
  https://github.com/matcool/gd-imgui-cocos  

---

## What is original to ZBOT-MOBILE

- Mobile-first UI (floating draggable button instead of keyboard toggle)
- Touch-optimized layout and controls
- "Perfect run only" auto-save system
- Spam-safe input normalization (`minHoldFrames`, `minGapFrames`)
- Built-in autoclicker / spammer with configurable behavior
- Optional exclusion of automated inputs from recordings
- Persistent settings using Geode save system
- Android32 / Android64 automated build pipeline

---

## Maintainer

- **pepitogumball** — mobile port, adaptation, and maintenance

---

## License / attribution notice

This project builds upon prior community work.  
If any original authors believe attribution is missing or incorrect,
please open an issue:

https://github.com/pepitogumball-lang/ZBOT-MOBILE

Credits will be updated accordingly or content will be adjusted if required.
