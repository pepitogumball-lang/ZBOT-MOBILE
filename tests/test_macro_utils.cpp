// Tests unitarios de la lógica pura de macros (src/macro_utils.hpp).
//
// No dependen de Geode, GDReplayFormat ni Geometry Dash: solo de la STL.
// Se compilan y ejecutan en CI (job "unit-tests") y también localmente:
//
//     g++ -std=c++23 -I src tests/test_macro_utils.cpp -o /tmp/test_macro && /tmp/test_macro
//
// Cubren los casos T-02..T-13 de docs/MACRO_TEST_PLAN.md.

#include "macro_utils.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Misma forma que gdr::Input.
struct In {
  std::uint32_t frame;
  int button;
  bool player2;
  bool down;
};

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    ++g_checks;                                                           \
    if (!(cond)) {                                                        \
      ++g_failures;                                                       \
      std::printf("FALLO en %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
    }                                                                     \
  } while (0)

static In mk(std::uint32_t frame, int button, bool p2, bool down) {
  return In{frame, button, p2, down};
}

static void test_sort(std::vector<In>& v) { zmacro::sortByFrame(v); }
static void test_dedupe(std::vector<In>& v) { zmacro::dedupeNoOps(v); }
static void test_purge(std::vector<In>& v, int f) { zmacro::purgeAfterFrame(v, f); }

// T-02 / T-04: un press y su release se conservan exactamente.
static void test_press_release_preserved() {
  std::vector<In> v = { mk(10, 1, false, true), mk(15, 1, false, false) };
  test_sort(v);
  test_dedupe(v);
  CHECK(v.size() == 2);
  CHECK(v[0].frame == 10 && v[0].down);
  CHECK(v[1].frame == 15 && !v[1].down);
}

// T-04: hold largo no se colapsa ni se desplaza.
static void test_long_hold() {
  std::vector<In> v = { mk(10, 1, false, true), mk(100, 1, false, false) };
  test_dedupe(v);
  CHECK(v.size() == 2);
  CHECK(v[0].frame == 10);
  CHECK(v[1].frame == 100);
}

// T-03: pulsaciones rápidas alternas se conservan todas.
static void test_rapid_presses() {
  std::vector<In> v = {
    mk(10, 1, false, true), mk(12, 1, false, false),
    mk(14, 1, false, true), mk(16, 1, false, false),
  };
  test_sort(v);
  test_dedupe(v);
  CHECK(v.size() == 4);
}

// T-05: eventos en el mismo frame se conservan (distinto botón).
static void test_same_frame_events() {
  std::vector<In> v = {
    mk(50, 1, false, true), mk(50, 2, false, true),
  };
  test_sort(v);
  test_dedupe(v);
  CHECK(v.size() == 2);
  CHECK(v[0].frame == 50 && v[1].frame == 50);
}

// T-06: frame 0 es válido.
static void test_frame_zero() {
  std::vector<In> v = { mk(0, 1, false, true) };
  test_sort(v);
  test_dedupe(v);
  CHECK(v.size() == 1);
  CHECK(v[0].frame == 0);
}

// T-09: frames no monotónicos se ordenan.
static void test_sort_nonmonotonic() {
  std::vector<In> v = { mk(100, 1, false, true), mk(50, 1, false, false),
                        mk(75, 1, false, true) };
  test_sort(v);
  CHECK(v.size() == 3);
  CHECK(v[0].frame == 50 && v[1].frame == 75 && v[2].frame == 100);
  CHECK(zmacro::framesMonotonic(v));
}

// T-12: no-ops redundantes se eliminan (mismo estado repetido).
static void test_dedupe_redundant() {
  std::vector<In> v = {
    mk(10, 1, false, true), mk(11, 1, false, true), mk(12, 1, false, true),
  };
  test_dedupe(v);
  CHECK(v.size() == 1);
  CHECK(v[0].frame == 10);
}

// Un release inicial (estado desconocido -> false) se conserva.
static void test_initial_release_preserved() {
  std::vector<In> v = { mk(5, 1, false, false) };
  test_dedupe(v);
  CHECK(v.size() == 1);
}

// Dual mode: el estado de P1 y P2 es independiente.
static void test_dual_mode_independent() {
  std::vector<In> v = {
    mk(10, 1, false, true),  // P1 press
    mk(10, 1, true,  true),  // P2 press (mismo botón, distinto jugador)
  };
  test_dedupe(v);
  CHECK(v.size() == 2);
}

// T-10: botón fuera de rango se conserva sin corromper la deduplicación.
static void test_out_of_range_button() {
  std::vector<In> v = { mk(10, 99, false, true), mk(11, 1, false, true) };
  test_dedupe(v);
  CHECK(v.size() == 2);
}

// T-11: los frames NUNCA se modifican (a diferencia del bug antiguo).
static void test_frames_never_modified() {
  std::vector<In> v = {
    mk(3, 1, false, true), mk(5, 1, false, false),
    mk(6, 2, false, true), mk(7, 2, false, false),
  };
  std::vector<std::uint32_t> before;
  for (auto& in : v) before.push_back(in.frame);
  test_sort(v);
  test_dedupe(v);
  CHECK(v.size() == before.size());
  for (std::size_t i = 0; i < v.size(); ++i) {
    CHECK(v[i].frame == before[i]);
  }
}

// T-13: purgeAfter elimina frames >= frame y conserva los anteriores.
static void test_purge_after() {
  std::vector<In> v = {
    mk(10, 1, false, true), mk(20, 1, false, false),
    mk(30, 1, false, true), mk(40, 1, false, false),
  };
  test_purge(v, 30);
  CHECK(v.size() == 2);
  CHECK(v[0].frame == 10 && v[1].frame == 20);
}

// purgeAfter(0) limpia todo (reinicio desde el principio).
static void test_purge_zero() {
  std::vector<In> v = { mk(0, 1, false, true), mk(1, 1, false, false) };
  test_purge(v, 0);
  CHECK(v.empty());
}

// Lista vacía no rompe nada.
static void test_empty() {
  std::vector<In> v;
  test_sort(v);
  test_dedupe(v);
  test_purge(v, 10);
  CHECK(v.empty());
  CHECK(zmacro::framesMonotonic(v));
}

int main() {
  test_press_release_preserved();
  test_long_hold();
  test_rapid_presses();
  test_same_frame_events();
  test_frame_zero();
  test_sort_nonmonotonic();
  test_dedupe_redundant();
  test_initial_release_preserved();
  test_dual_mode_independent();
  test_out_of_range_button();
  test_frames_never_modified();
  test_purge_after();
  test_purge_zero();
  test_empty();

  std::printf("Checks: %d, Fallos: %d\n", g_checks, g_failures);
  if (g_failures == 0) {
    std::printf("OK: todos los tests pasaron\n");
    return 0;
  }
  std::printf("ERROR: %d test(s) fallaron\n", g_failures);
  return 1;
}
