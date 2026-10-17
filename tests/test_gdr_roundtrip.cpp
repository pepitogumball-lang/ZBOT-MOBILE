// Test de round-trip real contra la libreria GDReplayFormat (gdr).
//
// A diferencia de test_macro_utils.cpp (que prueba la logica pura sin
// dependencias), este test compila contra el header real de GDReplayFormat y
// ejercita importData(exportData()) de verdad. Eso valida que el formato .gdr
// que usa el mod conserva frames, botones, jugador y estado exactos, y que un
// archivo corrupto lanza excepcion (que el mod captura en fromFile).
//
// Requiere el include de gdr. En CI se clona el commit fijado:
//   git clone https://github.com/maxnut/GDReplayFormat gdr
//   g++ -std=c++17 -I gdr/include tests/test_gdr_roundtrip.cpp -o /tmp/test_gdr
//
// Los structs imitan exactamente la forma de zInput/zReplay de src/replay.hpp.

#include <gdr/gdr.hpp>

#include <cstdint>
#include <cstdio>
#include <vector>

struct tInput : gdr::Input {
  tInput() = default;
  tInput(int frame, int button, bool player2, bool down)
    : Input(frame, button, player2, down) {}
};

struct tReplay : gdr::Replay<tReplay, tInput> {
  tReplay() : Replay("G-Macro-Android", "2.0.0") {}
};

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    ++g_checks;                                                         \
    if (!(cond)) {                                                      \
      ++g_failures;                                                     \
      std::printf("FALLO en %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
    }                                                                   \
  } while (0)

static tReplay makeReplay() {
  tReplay r;
  r.framerate = 240.f;
  r.levelInfo.id = 42;
  r.levelInfo.name = "Test Level";
  r.author = "tester";
  r.seed = 7;
  r.coins = 3;
  r.ldm = true;
  r.duration = 0.5f;
  r.inputs.push_back(tInput(0, 1, false, true));
  r.inputs.push_back(tInput(12, 1, false, false));
  r.inputs.push_back(tInput(12, 2, false, true));
  r.inputs.push_back(tInput(240, 1, true, true));
  r.inputs.push_back(tInput(300, 1, true, false));
  return r;
}

static void compare(tReplay const& a, tReplay const& b, const char* which) {
  CHECK(a.framerate == b.framerate);
  CHECK(a.levelInfo.id == b.levelInfo.id);
  CHECK(a.seed == b.seed);
  CHECK(a.coins == b.coins);
  CHECK(a.ldm == b.ldm);
  CHECK(a.botInfo.name == b.botInfo.name);
  CHECK(a.botInfo.version == b.botInfo.version);
  CHECK(a.inputs.size() == b.inputs.size());
  if (a.inputs.size() == b.inputs.size()) {
    for (std::size_t i = 0; i < a.inputs.size(); ++i) {
      CHECK(a.inputs[i].frame == b.inputs[i].frame);
      CHECK(a.inputs[i].button == b.inputs[i].button);
      CHECK(a.inputs[i].player2 == b.inputs[i].player2);
      CHECK(a.inputs[i].down == b.inputs[i].down);
    }
  }
  (void)which;
}

// T-07: round-trip binario (msgpack) sin perdida.
static void test_roundtrip_msgpack() {
  tReplay r = makeReplay();
  std::vector<std::uint8_t> data = r.exportData(false);
  CHECK(!data.empty());
  tReplay back = tReplay::importData(data);
  compare(r, back, "msgpack");
}

// Round-trip JSON.
static void test_roundtrip_json() {
  tReplay r = makeReplay();
  std::vector<std::uint8_t> data = r.exportData(true);
  CHECK(!data.empty());
  tReplay back = tReplay::importData(data);
  compare(r, back, "json");
}

// Macro larga (1000+ inputs) conserva todos los eventos.
static void test_large_replay() {
  tReplay r;
  r.framerate = 240.f;
  for (int i = 0; i < 1500; ++i) {
    r.inputs.push_back(tInput(i * 2, 1 + (i % 3), (i % 2) == 0, (i % 2) == 0));
  }
  std::vector<std::uint8_t> data = r.exportData(false);
  tReplay back = tReplay::importData(data);
  CHECK(back.inputs.size() == 1500);
  CHECK(back.inputs.front().frame == 0);
  CHECK(back.inputs.back().frame == 2998);
}

// Frames exactos: un click y su release no cambian de frame.
static void test_exact_frames() {
  tReplay r;
  r.inputs.push_back(tInput(10, 1, false, true));
  r.inputs.push_back(tInput(15, 1, false, false));
  std::vector<std::uint8_t> data = r.exportData(false);
  tReplay back = tReplay::importData(data);
  CHECK(back.inputs.size() == 2);
  CHECK(back.inputs[0].frame == 10 && back.inputs[0].down);
  CHECK(back.inputs[1].frame == 15 && !back.inputs[1].down);
}

// framerate por defecto 240 y frameForTime coherente (1 s = 240 frames).
static void test_framerate_240() {
  tReplay r;
  CHECK(r.framerate == 240.f);
  CHECK(r.frameForTime(1.0) == 240u);
  CHECK(r.frameForTime(0.0) == 0u);
}

// T-08: un archivo corrupto/truncado LANZA excepcion (el mod la captura).
static void test_corrupt_throws() {
  bool threw = false;
  try {
    std::vector<std::uint8_t> bad = { 0xFF, 0x00, 0x12, 0x34, 0x56, 0x78 };
    tReplay back = tReplay::importData(bad);
    (void)back;
  } catch (...) {
    threw = true;
  }
  CHECK(threw);

  threw = false;
  try {
    std::vector<std::uint8_t> empty;
    tReplay back = tReplay::importData(empty);
    (void)back;
  } catch (...) {
    threw = true;
  }
  CHECK(threw);
}

int main() {
  test_roundtrip_msgpack();
  test_roundtrip_json();
  test_large_replay();
  test_exact_frames();
  test_framerate_240();
  test_corrupt_throws();

  std::printf("Checks: %d, Fallos: %d\n", g_checks, g_failures);
  if (g_failures == 0) {
    std::printf("OK: todos los tests de round-trip pasaron\n");
    return 0;
  }
  std::printf("ERROR: %d test(s) fallaron\n", g_failures);
  return 1;
}
