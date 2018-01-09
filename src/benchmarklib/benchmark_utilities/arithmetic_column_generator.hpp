#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#include "all_type_variant.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
class ValueColumn;

}  // namespace opossum

namespace benchmark_utilities {

template <typename T>
class ArithmeticColumnGenerator {
  static_assert(std::is_arithmetic_v<T>);

 public:
  ArithmeticColumnGenerator(opossum::PolymorphicAllocator<size_t> alloc);

  void set_row_count(const uint32_t row_count);

  std::shared_ptr<opossum::ValueColumn<T>> uniformly_distributed_column(const T min, const T max) const;

  struct OutlierParams {
    const double fraction;
    const T mean;
    const T std_dev;
  };

  std::shared_ptr<opossum::ValueColumn<T>> normally_distributed_column(
      const T mean, const T std_dev, std::optional<OutlierParams> outlier_params) const;

 private:
  std::shared_ptr<opossum::ValueColumn<T>> column_from_values(opossum::pmr_concurrent_vector<T> values) const;

 private:
  const opossum::DataType _data_type;
  const opossum::PolymorphicAllocator<size_t> _alloc;
  uint32_t _row_count;
};

}  // benchmark_utilities
