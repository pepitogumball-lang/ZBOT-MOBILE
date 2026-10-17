#include "GMacro.hpp"
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>

// TPS efectivo del macro en curso. El formato .gdr define los frames por
// TIEMPO: gdr::Replay::frameForTime() es literalmente `time * framerate`, y el
// archivo guarda `framerate` (240 por defecto). Por eso el índice en vivo usa
// el framerate del macro cargado; sin macro, GMACRO_TPS (240). Para nuestras
// propias grabaciones createNewReplay() fija framerate = 240.0, así que en la
// práctica es 240, igual que xdBot/Eclipse.
double GMacro::getTps() {
  GMacro* mgr = GMacro::get();
  if (mgr->currentReplay && mgr->currentReplay->framerate > 0.f) {
    return static_cast<double>(mgr->currentReplay->framerate);
  }
  return GMACRO_TPS;
}

// Índice de frame del macro: FUENTE ÚNICA DE VERDAD para grabación y
// reproducción. NO se usa GJGameState::m_currentProgress.
//
// Por qué (medido en el dispositivo y contrastado con los bots de referencia):
// ZBOT implementa el speedhack escalando el dt del scheduler (speedhack.cpp:
// CCScheduler::update(dt * speed), exactamente el mismo método que el "clock
// speedhack" de xdBot). m_currentProgress avanza por pasos de física, y cuántos
// pasos caben por segundo de JUEGO depende del factor de velocidad; por eso un
// macro grabado a 0.25x quedaba indexado con números que NO corresponden a la
// misma posición del nivel al reproducirlo a 1x: los inputs se agolpaban al
// principio y el run moría (justo lo que reportó el usuario).
//
// m_levelTime acumula el dt YA escalado: es tiempo de JUEGO, no de reloj.
// Multiplicado por el framerate da un índice idéntico para una misma posición
// del nivel a cualquier velocidad. Es lo que usan:
//   - el propio formato .gdr: gdr::Replay::frameForTime(t) = t * framerate
//   - xdBot  global.cpp:      m_levelTime * getTPS()   (macros nativos)
//   - OpenHack zephyrus:      m_levelTime * 240        (motor de replay)
//   - Eclipse variables.cpp:  "frameReal" = m_levelTime * TPS
//   - silicate updater.cpp:   m_currentProgress / 2     (misma escala: a 240 Hz
//                             m_currentProgress avanza 2 por tick de física)
//
// m_levelTime vive en GJGameState, que CheckpointObject guarda y restaura, así
// que tras un checkpoint grabar y reproducir ven el mismo valor (silicate lo
// verifica en tests/port.cpp).
int GMacro::getFrame(GJBaseGameLayer* layer) {
  GJBaseGameLayer* l = layer ? layer : PlayLayer::get();
  if (!l) return 0;
  double t = l->m_gameState.m_levelTime;
  if (t < 0.0) t = 0.0;
  return static_cast<int>(t * GMacro::getTps()) + GMacro::get()->frameOffset;
}

//
// emission. We:
//
void GMacro::playSound(bool /*p2*/, int /*button*/, bool down) {
  if (!down) return;

  auto* engine = FMODAudioEngine::sharedEngine();
  if (!engine) return;

  engine->playEffect("playSound_01.ogg");
}
