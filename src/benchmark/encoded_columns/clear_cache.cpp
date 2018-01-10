#include "clear_cache.hpp"

#include <vector>

namespace opossum {

uint32_t clear_cache() {
  static const auto l3_cache_size_in_byte = 500 * 1024 * 1024;

  auto sum = uint32_t{0u};

  for (auto i = 0u; i < 4u; ++i) {
    auto clear_vector = std::vector<uint32_t>();
    clear_vector.resize(l3_cache_size_in_byte / sizeof(int), 13);
    for (auto& x : clear_vector) {
      x += 1;
      sum += x;
    }
    clear_vector.resize(0);
  }

  return sum;
}

}  // namespace opossum
