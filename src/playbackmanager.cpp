#include "GMacro.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <cocos2d.h>
#include <cmath>
#include <algorithm>
#include "logger.hpp"

using namespace geode::prelude;

class $modify(zPlayGJBGL, GJBaseGameLayer) {
  struct Fields {
    int currIndex   = 0;
    int clickBotIndex = 0;
    int   lastEpoch = -1;
    zReplay* lastReplay = nullptr;
    bool spamHeld[2]  = { false, false };
    int spamAnchor  = -1;
    bool spamWasOn   = false;
    int lastSeenFrame = -1;        // diagnóstico de frame congelado
    int sameFrameCount = 0;
    bool warnedStuck = false;
    int tickCall = 0;              // limitador del log de estado
  };

// Core playback and spam logic loop
  void processCommands(float dt, bool isHalfTick, bool isLastTick) {
    GMacro* mgr = GMacro::get();
    auto* pl = PlayLayer::get();
    bool inLevel = pl != nullptr;

    //
    if (mgr->frameAdvance && inLevel && !mgr->doAdvance) {
      mgr->doAdvance = false;
      return; 
    }
    mgr->doAdvance = false;

    // El processCommands original se ejecuta ANTES de replicar los inputs.
    // Es el mismo orden que usan Eclipse, zBot y ToastyReplay: así la lectura
    // del contador ocurre en la misma fase que durante la grabación y no
    // aparece el clásico desfase de un frame entre grabar y reproducir.
    GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

    // CONTADOR DE FRAMES: tiempo de juego (m_levelTime * 240), NO
    // m_currentProgress ni m_currentStep. El motivo completo —speedhack a
    // 0.25x que desincronizaba la reproducción a 1x— está en el comentario de
    // GMacro::getFrame(). Debe ser EXACTAMENTE el mismo índice que usa la
    // grabación o el macro no coincide nunca.
    int frame = GMacro::getFrame(this);

    //
    if (mgr->state == PLAYBACK && mgr->currentReplay) {
      bool epochChanged  = (m_fields->lastEpoch != mgr->playbackEpoch);
// Consistency checks for playback state
      bool replayChanged = (m_fields->lastReplay != mgr->currentReplay);
      bool justLoaded = mgr->justLoaded;

      if (epochChanged || replayChanged || justLoaded) {
        if (justLoaded) mgr->justLoaded = false;
        m_fields->lastEpoch   = mgr->playbackEpoch;
        m_fields->lastReplay  = mgr->currentReplay;
        m_fields->currIndex   = 0;
        m_fields->clickBotIndex = 0; // Removed: ghost flag

        // `lt` (tiempo de juego) es ahora el índice real; `step`/`prog` quedan
        // como contraste de diagnóstico. Al aparecer desde un checkpoint,
        // m_levelTime —miembro de GJGameState, el estado que CheckpointObject
        // restaura— vuelve al valor del checkpoint y el cursor se realinea con él.
        ZLOG_INFO("EPOCH", "cursor reset"
           " | frame=" << frame
           << " | inLevel=" << inLevel
           << " | inputs=" << mgr->currentReplay->inputs.size()
           << " | hasCheckpoint=" << (inLevel && pl->m_currentCheckpoint != nullptr)
           << " | lt=" << m_gameState.m_levelTime
           << " step=" << m_currentStep
           << " prog=" << m_gameState.m_currentProgress
           << " | epoch=" << mgr->playbackEpoch);

// Handle mid-level playback synchronization
        bool isMidLevelJoin = inLevel
          && (pl->m_currentCheckpoint != nullptr || frame > 2)
          && frame > 1;
        if (isMidLevelJoin || justLoaded) {
          auto& inputs = mgr->currentReplay->inputs;
          // Mismo criterio que tras un respawn: saltar todo lo anterior al
          // frame actual pero conservar el input que cae justo en este frame.
          m_fields->currIndex = static_cast<int>(
            zmacro::firstNotDue(inputs, 0,
                static_cast<std::int64_t>(frame) - 1));
          m_fields->clickBotIndex = m_fields->currIndex;
        }
      }

      auto& inputs = mgr->currentReplay->inputs;

      // Guardas de seguridad: no se replican inputs mientras el jugador esta
      // muerto o mientras corre la animacion de fin de nivel. Repartir inputs
      // en esas escenas deja el bot "colgado" (inputs fantasma tras el respawn
      // o clicks del clickbot durante la animacion de muerte). El cursor se
      // re-alinea en resetLevel cuando vuelve a jugarse.
      bool canFire = inLevel
        && !(m_player1 && m_player1->m_isDead)
        && !pl->m_levelEndAnimationStarted;

      // Valores seguros para el diagnostico: `pl` puede ser nulo cuando el
      // hook corre fuera de un nivel, y hay que poder imprimirlos igual.
      const bool dbgDead    = (m_player1 && m_player1->m_isDead);
      const bool dbgEndAnim = inLevel && pl->m_levelEndAnimationStarted;
      const bool dbgPaused  = inLevel && pl->m_isPaused;

      // Regla única de disparo (zmacro::firstNotDue, con tests unitarios).
      std::size_t due = canFire
        ? zmacro::firstNotDue(inputs,
              static_cast<std::size_t>(m_fields->currIndex),
              static_cast<std::int64_t>(frame))
        : static_cast<std::size_t>(m_fields->currIndex);

      for (; static_cast<std::size_t>(m_fields->currIndex) < due;
           m_fields->currIndex++) {
        auto& input = inputs[m_fields->currIndex];

        // En Android el log va a un archivo: registrar cada input de un macro
        // con spam genera megas de texto y contención de I/O. Se guardan los
        // primeros 24 de cada epoch (lo justo para diagnosticar el clásico
        // "primer salto perdido") y después 1 de cada 100.
        if (m_fields->currIndex < 24 || (m_fields->currIndex % 100) == 0) {
          // `step` vs `prog` es el diagnóstico de B-01: permite confirmar en el
          // dispositivo si GJBaseGameLayer::m_currentStep y
          // GJGameState::m_currentProgress coinciden o divergen (p. ej. al
          // reaparecer desde un checkpoint, ya que GJGameState es el estado que
          // CheckpointObject guarda y restaura).
          ZLOG_INFO("PLAYBACK", "FIRE"
             " frame=" << input.frame
             << " btn=" << input.button
             << " down=" << input.down
             << " p2=" << input.player2
             << " | curFrame=" << frame
             << " | lt=" << m_gameState.m_levelTime
             << " step=" << m_currentStep
             << " prog=" << m_gameState.m_currentProgress
             << " | idx=" << (m_fields->currIndex));
        }
        #ifdef GEODE_IS_MOBILE
        m_allowedButtons.clear();
        #endif
        GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
      }

      // Diagnóstico (no correctivo): si el frame del juego no avanza durante
      // muchos ciclos en PLAYBACK, es señal de pausa o de un contador roto.
      // NO se toca el cursor: hacerlo re-ejecutaría inputs ya pasados.
      bool frameFrozen = (frame == m_fields->lastSeenFrame);
      if (frameFrozen) {
        m_fields->sameFrameCount++;
        if (m_fields->sameFrameCount > 240 && !m_fields->warnedStuck) {
          ZLOG_WARN("PLAYBACK", "el frame del juego no avanza"
             << " | frame=" << frame
             << " | currIndex=" << m_fields->currIndex
             << " | step=" << m_currentStep
             << " prog=" << m_gameState.m_currentProgress
             << " | canFire=" << (canFire ? 1 : 0)
             << " dead=" << (dbgDead ? 1 : 0)
             << " endAnim=" << (dbgEndAnim ? 1 : 0)
             << " paused=" << (dbgPaused ? 1 : 0));
          m_fields->warnedStuck = true;
        }
      } else {
        m_fields->lastSeenFrame = frame;
        m_fields->sameFrameCount = 0;
        m_fields->warnedStuck = false;
      }

      // Log de estado rate-limitado POR LLAMADAS, no por `frame % 60`.
      // Con el contador clavado en 0, `0 % 60 == 0` era siempre cierto y el
      // log se inundaba con cientos de líneas idénticas por segundo.
      if ((m_fields->tickCall++ % 240) == 0) {
        int remaining = (int)inputs.size() - m_fields->currIndex;
        auto lvl = (remaining == 0 && !inputs.empty())
              ? ZLogLevel::Warn : ZLogLevel::Debug;
        ZBotLogger::get().log(lvl, "PLAYBACK",
          std::string("tick frame=") + std::to_string(frame)
          + " cursor=" + std::to_string(m_fields->currIndex)
          + "/" + std::to_string((int)inputs.size())
          + " remaining=" + std::to_string(remaining)
          + " | lt=" + std::to_string(m_gameState.m_levelTime)
          + " step=" + std::to_string(m_currentStep)
          + " prog=" + std::to_string(m_gameState.m_currentProgress)
          + " | canFire=" + std::to_string(canFire ? 1 : 0)
          + " dead=" + std::to_string(dbgDead ? 1 : 0)
          + " endAnim=" + std::to_string(dbgEndAnim ? 1 : 0)
          + " paused=" + std::to_string(dbgPaused ? 1 : 0)
          + " frameAdv=" + std::to_string(mgr->frameAdvance ? 1 : 0));
      }

      // Clickbot: reproducir sonido en el mismo frame del input (sin offset)
      // para que el audio coincida con la acción visual.
      if (canFire) {
        std::size_t clickDue = zmacro::firstNotDue(inputs,
            static_cast<std::size_t>(m_fields->clickBotIndex),
            static_cast<std::int64_t>(frame));
        for (; static_cast<std::size_t>(m_fields->clickBotIndex) < clickDue;
             m_fields->clickBotIndex++) {
          auto& click = inputs[m_fields->clickBotIndex];
          if (mgr->clickbotEnabled) {
            mgr->playSound(click.player2, click.button, click.down);
          }
        }
      }
    } else {
      m_fields->lastEpoch = mgr->playbackEpoch;
      m_fields->lastReplay = mgr->currentReplay;
    }

    // (el processCommands original ya se ejecutó al inicio de este hook)

    //
    bool spamGateOk = true;
    if (mgr->spamOnlyDuringPlay && inLevel) {
      spamGateOk = !pl->m_isPaused;
    }

    if (mgr->spamEnabled && inLevel && spamGateOk
        && !(m_player1 && m_player1->m_isDead)
        && !pl->m_levelEndAnimationStarted) {
      if (!m_fields->spamWasOn || frame < m_fields->spamAnchor) {
        m_fields->spamAnchor = frame;
        m_fields->spamHeld[0] = false;
        m_fields->spamHeld[1] = false;
      }
      m_fields->spamWasOn = true;

      // El frame del juego está indexado por tiempo de juego con el mismo TPS
      // que getFrame (framerate del macro, o 240 sin macro), así que el ciclo
      // del spam se mide en esas mismas unidades para que el CPS sea real.
      double tps = GMacro::getTps();
      double cps = std::clamp(mgr->spamCPS, 0.1, 240.0);
      double cycle = tps / cps;
      if (cycle < 1.0) cycle = 1.0;
      double hold = cycle * std::clamp(mgr->spamHoldRatio, 0.0, 1.0);
      if (hold < 1.0 && mgr->spamHoldRatio > 0.0) hold = 1.0;
      if (hold >= cycle) hold = cycle - 0.001;

      double rel = static_cast<double>(frame - m_fields->spamAnchor);
      double phase = std::fmod(rel, cycle);
      if (phase < 0.0) phase += cycle;
// Consistency checks for playback state
      bool wantDown = (phase < hold);

      bool dual = m_gameState.m_isDualMode &&
            m_levelSettings &&
            m_levelSettings->m_twoPlayerMode;

      int button = mgr->spamButton;
      if (button < 1 || button > 3) button = 1;

// Player button drive logic
      auto driveButton = [&](int playerIdx, bool isP2) {
        bool& cur = m_fields->spamHeld[playerIdx];
        if (cur == wantDown) return;
        cur = wantDown;

        mgr->spamSuppressRecord = true;
        #ifdef GEODE_IS_MOBILE
        m_allowedButtons.clear();
        #endif
        GJBaseGameLayer::handleButton(wantDown, button, !isP2);
        mgr->spamSuppressRecord = false;
      };

      // spamPlayer: 0=P1, 1=P2, 2=Both
      // spamDual solo aplica si el nivel esta en modo dual
      bool doP1 = (mgr->spamPlayer == 0 || mgr->spamPlayer == 2);
      bool doP2 = (mgr->spamPlayer == 1 || mgr->spamPlayer == 2) && dual;
      if (doP1) driveButton(0, false);
      if (doP2) driveButton(1, true);
    } else {
      m_fields->spamWasOn = false;
    }
  }
};

class $modify(zPlayPL, PlayLayer) {
  void resetLevel() {
    PlayLayer::resetLevel();

    GMacro* mgr = GMacro::get();
    auto* gjbgl = static_cast<GJBaseGameLayer*>(this);
    auto* self  = static_cast<zPlayGJBGL*>(gjbgl);
    auto* playLayer = PlayLayer::get();

    if (mgr->state == PLAYBACK && mgr->currentReplay && playLayer == this) {
      auto& f = self->m_fields;
      f->lastEpoch = mgr->playbackEpoch;
      f->lastReplay = mgr->currentReplay;
      f->currIndex = 0;
      f->clickBotIndex = 0;
      f->lastSeenFrame = -1;
      f->sameFrameCount = 0;
      f->warnedStuck = false;
      f->tickCall = 0;

      // Mismo contador que en processCommands (GMacro::getFrame). Usar aquí el
      // mismo índice que al reproducir es lo que garantiza que el cursor quede
      // bien realineado tras un respawn o al aparecer desde un checkpoint.
      int frame = GMacro::getFrame(self);
      auto& inputs = mgr->currentReplay->inputs;
      // Misma regla que la grabación y el disparo: saltarse lo anterior al
      // frame actual, dejando el cursor justo donde empieza a reproducir.
      f->currIndex = static_cast<int>(
        zmacro::firstNotDue(inputs, 0,
            static_cast<std::int64_t>(frame) - 1));
      f->clickBotIndex = f->currIndex;
    }
  }
};
