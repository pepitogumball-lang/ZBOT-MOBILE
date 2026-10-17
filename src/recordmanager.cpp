#include "GMacro.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Notification.hpp>
#include "logger.hpp"

using namespace geode::prelude;

bool shouldRecord(GMacro* mgr, bool down, int button, bool p2, int frame) {
  // Deduplicación exacta: mismo frame, misma tecla, mismo jugador, mismo estado = evento duplicado
  if (frame == mgr->dedupeLastFrame && down == mgr->dedupeLastDown
      && button == mgr->dedupeLastButton && p2 == mgr->dedupeLastP2) {
    return false;
  }

  if (mgr->dedupeInputs) {
    if (button >= 0 && button <= 7) { // incluir boton 0 por si acaso
      bool& held = p2 ? mgr->p2ButtonHeld[button] : mgr->p1ButtonHeld[button];
      if (held == down) return false;
      held = down;
    }
  } else if (button >= 0 && button <= 7) {
    // Aunque la deduplicacion este desactivada, hay que MANTENER el estado de
    // botones: se usa para emitir los "release" al reiniciar el nivel
    // (resetLevel). Si no se actualiza, esos releases se pierden.
    bool& held = p2 ? mgr->p2ButtonHeld[button] : mgr->p1ButtonHeld[button];
    held = down;
  }

  mgr->dedupeLastFrame   = frame;
  mgr->dedupeLastDown    = down;
  mgr->dedupeLastButton  = button;
  mgr->dedupeLastP2      = p2;
  return true;
}

void resetDedupeState(GMacro* mgr) {
  mgr->dedupeLastFrame   = -1;
  mgr->dedupeLastDown    = false;
  mgr->dedupeLastButton  = -1;
  mgr->dedupeLastP2      = false;
}

class $modify(zRecGJBGL, GJBaseGameLayer) {
  void handleButton(bool down, int button, bool p1) {
    GJBaseGameLayer::handleButton(down, button, p1);

    GMacro* mgr = GMacro::get();
    if (mgr->state != RECORD || !mgr->currentReplay) return;

    if (mgr->spamSuppressRecord && !mgr->spamRecordToMacro) return;

// Determine if this is a Player 2 input in Dual Mode
    bool p2 = !p1 && m_levelSettings && m_levelSettings->m_twoPlayerMode
         && this->m_gameState.m_isDualMode;
    
    int frame = m_currentStep;
    
    if (!shouldRecord(mgr, down, button, p2, frame)) return;

    mgr->currentReplay->addInput(
      frame,
      button, p2, down
    );

    ZLOG_INFO("RECORD", (down ? "◢" : "◤") 
      << (p2 ? "P2" : "P1") 
      << " | frame=" << (frame)
      << " | btn=" << button);
  }
};

class $modify(zRecPL, PlayLayer) {
  bool init(GJGameLevel* lvl, bool useReplay, bool dontCreateObjects) {
    GMacro* mgr = GMacro::get();

    if (mgr->state == RECORD) {
      mgr->createNewReplay(lvl);
      resetDedupeState(mgr); // resetear para que no se coma el primer click
    }

    if (mgr->state == PLAYBACK) {
      mgr->requestPlaybackRestart();
    }

    if (!PlayLayer::init(lvl, useReplay, dontCreateObjects)) return false;

    if (mgr->autoSafeMode) {
      m_isPracticeMode = true;
    }

    return true;
  }

// Handle practice mode and reset logic
  void resetLevel() {
    PlayLayer::resetLevel();

    GMacro* mgr = GMacro::get();
    if (mgr->state != RECORD || !mgr->currentReplay) return;

    int frame = m_currentStep;
    mgr->currentReplay->purgeAfter(frame);

    mgr->resetButtonStateAfterFrame(frame);

// Check for dual mode configuration
    bool dual = this->m_gameState.m_isDualMode &&
          m_levelSettings &&
          m_levelSettings->m_twoPlayerMode;

    auto releaseAllHeld = [&](bool* held, bool p2) {
      for (int b = 0; b <= 7; ++b) { // incluir boton 0 por si acaso
        if (!held[b]) continue;
        mgr->currentReplay->addInput(frame, b, p2, false);
        held[b] = false;
      }
    };
    releaseAllHeld(mgr->p1ButtonHeld, false);
    if (dual) releaseAllHeld(mgr->p2ButtonHeld, true);

    if (m_player1) m_player1->m_isDashing = false;
    if (dual && m_player2) m_player2->m_isDashing = false;
  }

  void destroyPlayer(PlayerObject* player, GameObject* obj) {
    GMacro* mgr = GMacro::get();
    if (mgr->autoSafeMode && (mgr->state == RECORD || mgr->state == PLAYBACK)) {
      return;
    }
    PlayLayer::destroyPlayer(player, obj);
  }

// Handle practice mode and reset logic
  void levelComplete() {
    GMacro* mgr = GMacro::get();
    if (mgr->state == RECORD && mgr->currentReplay) {
      mgr->levelCompleted = true;
      if (mgr->autoSave) {
        bool ok = mgr->currentReplay->save(mgr->minHoldFrames, mgr->minGapFrames);
        if (ok) {
          mgr->autoSavedThisRun = true;
          Notification::create("Macro perfecto guardado",
            NotificationIcon::Success, 1.5f)->show();
        } else {
          Notification::create("Error al guardar - revisa permisos de almacenamiento",
            NotificationIcon::Error, 2.0f)->show();
        }
      }
    }

    if (mgr->autoSafeMode) {
      Notification::create("Safe mode activo :3", NotificationIcon::Success, 1.5f)->show();
      return;
    }

    PlayLayer::levelComplete();
  }

// esto lo hizo Alberto a las 3 am
  void onExit() {
    GMacro* mgr = GMacro::get();
    if (mgr->state == RECORD && mgr->currentReplay && mgr->autoSave
      && !mgr->autoSavedThisRun) {
      if (!mgr->perfectRunOnly || mgr->levelCompleted) {
        bool ok = mgr->currentReplay->save(mgr->minHoldFrames, mgr->minGapFrames);
        if (ok) {
          mgr->autoSavedThisRun = true;
        }
      }
    }
    mgr->autoSavedThisRun = false;
    PlayLayer::onExit();
  }
};
