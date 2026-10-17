#ifndef _macro_utils_hpp
#define _macro_utils_hpp

// Utilidades puras de manipulación de inputs de macro.
//
// Este header NO depende de Geode ni de GDReplayFormat: solo de la STL. Así
// puede compilarse y testearse de forma aislada (ver tests/test_macro_utils.cpp),
// sin necesidad del SDK de Geode ni del juego.
//
// Las plantillas aceptan cualquier tipo con los miembros:
//   .frame   (entero, > 0; puede ser uint32_t)
//   .button  (int)
//   .player2 (bool)
//   .down    (bool)
// que es exactamente la forma de gdr::Input.

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace zmacro {

// Ordena por frame de forma estable (preserva el orden relativo de eventos en
// el mismo frame, algo importante para dual mode / misma-tecla).
template <class T>
void sortByFrame(std::vector<T>& v) {
  std::stable_sort(v.begin(), v.end(),
    [](const T& a, const T& b) {
      return static_cast<std::uint64_t>(a.frame) <
             static_cast<std::uint64_t>(b.frame);
    });
}

// Elimina eventos "no-op": un press (down=true) o release (down=false) que
// repite el estado ya activo para ese botón/jugador es redundante.
//
// IMPORTANTE: no modifica ningún frame. Solo descarta eventos redundantes.
// El estado inicial de cada botón es "desconocido" (-1), por lo que el primer
// evento de cada botón siempre se conserva (incluido un release inicial).
template <class T>
void dedupeNoOps(std::vector<T>& v) {
  int held[2][8];
  for (int p = 0; p < 2; ++p)
    for (int b = 0; b < 8; ++b)
      held[p][b] = -1;

  std::vector<T> out;
  out.reserve(v.size());

  for (auto const& in : v) {
    int p = in.player2 ? 1 : 0;
    int b = static_cast<int>(in.button);
    if (b < 0 || b >= 8) {
      // Botón fuera de rango: conservar sin tocar el estado de deduplicación.
      out.push_back(in);
      continue;
    }
    int prev = held[p][b];
    int now = in.down ? 1 : 0;
    if (prev == now) continue; // no-op redundante
    held[p][b] = now;
    out.push_back(in);
  }

  v = std::move(out);
}

// Elimina todos los eventos con frame >= frame.
template <class T>
void purgeAfterFrame(std::vector<T>& v, int frame) {
  v.erase(std::remove_if(v.begin(), v.end(),
    [frame](const T& in) {
      return static_cast<std::int64_t>(in.frame) >= static_cast<std::int64_t>(frame);
    }), v.end());
}

// Devuelve true si los frames son no decrecientes.
template <class T>
bool framesMonotonic(std::vector<T> const& v) {
  for (std::size_t i = 1; i < v.size(); ++i) {
    if (static_cast<std::uint64_t>(v[i].frame) <
        static_cast<std::uint64_t>(v[i - 1].frame)) {
      return false;
    }
  }
  return true;
}

// Sanea una lista cargada de disco: ordena por frame. (Los frames son
// unsigned, así que no hay negativos que corregir.)
template <class T>
void sanitize(std::vector<T>& v) {
  sortByFrame(v);
}

} // namespace zmacro

#endif
