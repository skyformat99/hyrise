#include "arithmetic_column_generator.hpp"

#include <random>
#include <memory>
#include <algorithm>

#include "resolve_type.hpp"
#include "storage/value_column.hpp"


namespace benchmark_utilities {

namespace {

template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
auto get_uniform_dist(const T min, const T max) {
  return std::uniform_int_distribution{min, max};
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
auto get_uniform_dist(const T min, const T max) {
  return std::uniform_real_distribution{min, max};
}

}  // namespace

using namespace opossum;

template <typename T>
ArithmeticColumnGenerator<T>::ArithmeticColumnGenerator(PolymorphicAllocator<size_t> alloc)
    : _data_type{data_type_from_type<T>()},
      _alloc{alloc},
      _row_count{1'000'000},
      _sorted{false} {}

template <typename T>
void ArithmeticColumnGenerator<T>::set_row_count(const uint32_t row_count) {
  _row_count = row_count;
}

template <typename T>
void ArithmeticColumnGenerator<T>::set_sorted(bool sorted) {
  _sorted = sorted;
}

template <typename T>
std::shared_ptr<ValueColumn<T>> ArithmeticColumnGenerator<T>::uniformly_distributed_column(const T min, const T max) const {
  auto values = pmr_concurrent_vector<T>(_row_count, _alloc);

  std::mt19937 gen{};
  auto dist = get_uniform_dist(min, max);

  for (auto i = 0u; i < _row_count; ++i) {
    values[i] = dist(gen);
  }

  if (_sorted) {
    std::sort(values.begin(), values.end());
  }

  return column_from_values(std::move(values));
}

template <typename T>
std::shared_ptr<ValueColumn<T>> ArithmeticColumnGenerator<T>::normally_distributed_column(const T mean, const T std_dev, std::optional<OutlierParams> outlier_params) const {
  auto values = pmr_concurrent_vector<T>(_row_count, _alloc);

  std::mt19937 gen{};
  auto dist = std::normal_distribution<double>(mean, std_dev);

  if (!outlier_params) {
    for (auto i = 0u; i < _row_count; ++i) {
      values[i] = dist(gen);
    }
  } else {
    auto is_outlier_dist = std::bernoulli_distribution(outlier_params->fraction);
    auto outlier_dist = std::normal_distribution<double>(outlier_params->mean, outlier_params->std_dev);

    for (auto i = 0u; i < _row_count; ++i) {
      if (is_outlier_dist(gen)) {
        values [i] = outlier_dist(gen);
      } else {
        values[i] = dist(gen);
      }
    }
  }

  if (_sorted) {
    std::sort(values.begin(), values.end());
  }

  return column_from_values(std::move(values));
}

template <typename T>
std::shared_ptr<ValueColumn<T>> ArithmeticColumnGenerator<T>::column_from_values(pmr_concurrent_vector<T> values) const {
  return std::allocate_shared<ValueColumn<T>>(_alloc, std::move(values));
}

template class ArithmeticColumnGenerator<int32_t>;
template class ArithmeticColumnGenerator<int64_t>;
template class ArithmeticColumnGenerator<float>;
template class ArithmeticColumnGenerator<double>;

}  // namespace benchmark_utilities
