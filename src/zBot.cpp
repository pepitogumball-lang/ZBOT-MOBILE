#include "zBot.hpp"
#include <Geode/binding/FMODAudioEngine.hpp>

// Clickbot SFX: play a short sound effect when the playback hook is
// about to fire a click. Previously a stub, which made the in-GUI
// "Clickbot SFX" toggle a no-op.
//
// The caller in playbackmanager.cpp already gates this on
// `clickbotEnabled`, so this function only handles the actual sound
// emission. We:
//   - skip "release" events so each press makes one click (otherwise
//     every input fires the SFX twice);
//   - route through FMODAudioEngine, which is how every other GD mod
//     plays one-shot effects;
//   - no-op silently if FMOD is not initialised yet (e.g. during very
//     early boot) so we never crash a frame.
//
// We don't bundle our own click sample. FMOD's playEffect resolves
// the filename against GD's audio search paths, so a missing file is
// a silent no-op rather than a hard error — safe behaviour on any
// install where the named asset is unavailable.
void zBot::playSound(bool /*p2*/, int /*button*/, bool down) {
    if (!down) return;

    auto* engine = FMODAudioEngine::sharedEngine();
    if (!engine) return;

    engine->playEffect("playSound_01.ogg");
}
