#include <cstdint>
#include <fstream>
#include <functional>
#include <sstream>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

#include "json.hpp"

#include "storage/column_encoders/utils.hpp"
#include "storage/column_encoders/base_column_encoder.hpp"
#include "storage/encoded_columns/utils.hpp"
#include "storage/encoded_columns/column_encoding_type.hpp"
#include "storage/iterables/create_iterable_from_column.hpp"
#include "storage/encoded_columns/utils.hpp"
#include "storage/zero_suppression/zs_type.hpp"
#include "storage/chunk_encoder.hpp"

#include "benchmark_utilities/arithmetic_column_generator.hpp"

#include "benchmark_memory_resource.hpp"
#include "benchmark_state.hpp"

namespace opossum {

namespace {

std::string to_string(EncodingType encoding_type) {
  static const auto string_for_type = std::map<EncodingType, std::string>{
    { EncodingType::Invalid, "Unencoded" },
    { EncodingType::Dictionary, "Dictionary" },
    { EncodingType::DeprecatedDictionary, "Dictionary (Deprecated)" },
    { EncodingType::RunLength, "Run Length" }};

  return string_for_type.at(encoding_type);
}

std::string to_string(ZsType zs_type) {
  static const auto string_for_type = std::map<ZsType, std::string>{
    { ZsType::FixedSizeByteAligned, "Fixed-size byte-aligned" },
    { ZsType::SimdBp128, "SIMD-BP128" }};

  return string_for_type.at(zs_type);
}

std::string to_string(const ColumnEncodingSpec& spec) {
  auto stream = std::stringstream{};
  stream << to_string(spec.encoding_type);
  if (spec.zs_type) {
    stream << " (" << to_string(*spec.zs_type) << ")";
  }
  return stream.str();
}

}  // namespace

class SingleDistributionBenchmark {
 public:
   static const auto row_count = 1'000'000;
   static const auto sorted = true;
   static constexpr auto name = "Uniform from 0 to 10.000";

 public:
  SingleDistributionBenchmark() {
    static const auto numa_node = 1;
    _memory_resource = std::make_unique<BenchmarkMemoryResource>(numa_node);
  }

 private:
  auto _distribution_generator() const {
    auto alloc = PolymorphicAllocator<size_t>{_memory_resource.get()};

    auto column_generator = benchmark_utilities::ArithmeticColumnGenerator<int32_t>{alloc};
    column_generator.set_row_count(row_count);
    column_generator.set_sorted(sorted);

    auto generator = [cg = column_generator]() { return cg.uniformly_distributed_column(0, 10'000); };

    using ValueColumnPtr = std::shared_ptr<ValueColumn<int32_t>>;
    return std::function<ValueColumnPtr()>{generator};
  }

  std::vector<ColumnEncodingSpec> _encoding_specs() {
    return { {EncodingType::DeprecatedDictionary}, {EncodingType::RunLength}, {EncodingType::Dictionary, ZsType::FixedSizeByteAligned}, {EncodingType::Dictionary, ZsType::SimdBp128} };
  }

  void _create_report() const {
    nlohmann::json benchmarks;

    for (const auto& result_set : _result_sets) {
      const auto& results = result_set.results;

      auto results_in_ms = std::vector<uint32_t>(result_set.results.size());
      std::transform(results.cbegin(), results.cend(), results_in_ms.begin(),
                     [](auto x) { return std::round(static_cast<double>(row_count) / std::chrono::duration_cast<std::chrono::microseconds>(x).count()); });

      nlohmann::json benchmark{
        {"encoding_spec", to_string(result_set.encoding_spec)},
        {"iterations", result_set.num_iterations},
        {"allocated_memory", result_set.allocated_memory},
        {"results", results_in_ms}};

      benchmarks.push_back(std::move(benchmark));
    }

    /**
     * Generate YY-MM-DD hh:mm::ss
     */
    auto current_time = std::time(nullptr);
    auto local_time = *std::localtime(&current_time);
    std::stringstream timestamp_stream;
    timestamp_stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    nlohmann::json context{
        {"date", timestamp_stream.str()},
        {"build_type", IS_DEBUG ? "debug" : "release"},
        {"distribution", name},
        {"row_count", row_count},
        {"sorted", sorted}};

    nlohmann::json report{{"context", context}, {"benchmarks", benchmarks}};

    auto output_file = std::ofstream("/Users/maxjendruk/Development/hyrise-jupyter/single_distribution.json");
    output_file << std::setw(2) << report << std::endl;
  }

  template <typename Functor>
  auto memory_consumption(Functor functor) {
    const auto allocated_before = _memory_resource->currently_allocated();
    auto result = functor();
    const auto allocated_after = _memory_resource->currently_allocated();
    const auto allocated_memory = allocated_after - allocated_before;
    return std::make_pair(std::move(result), allocated_memory);
  }

  const BenchmarkState benchmark_decompression_with_iterable(const BaseColumn& base_column) {
    static const auto max_num_iterations = 30u;
    static const auto max_duration = std::chrono::seconds{20};

    auto benchmark_state = BenchmarkState{max_num_iterations, max_duration};

    resolve_column_type<int32_t>(base_column, [&](auto& typed_column) {
      while (benchmark_state.keep_running()) {
        benchmark_state.measure([&]() {
          auto iterable = create_iterable_from_column<int32_t>(typed_column);

          auto sum = 0;
          iterable.for_each([&](auto value) {
            sum += value.value();
          });
        });
      }
    });

    return benchmark_state;
  }

 public:
  void run() {
    const auto generator = _distribution_generator();

    std::cout << "Begin Encoding Type: Unencoded" << std::endl;

    auto [value_column, allocated_memory] = memory_consumption([g = generator]() { return g(); });

    auto benchmark_state = benchmark_decompression_with_iterable(*value_column);

    auto results = benchmark_state.results();
    auto num_iterations = benchmark_state.num_iterations();
    _result_sets.push_back({{EncodingType::Invalid}, num_iterations, allocated_memory, std::move(results)});

    for (const auto& encoding_spec : _encoding_specs()) {

      std::cout << "Begin Encoding Type: " << to_string(encoding_spec) << std::endl;

      auto encoder = create_encoder(encoding_spec.encoding_type);

      if (encoding_spec.zs_type) {
        encoder->set_zs_type(*encoding_spec.zs_type);
      }

      auto [encoded_column, allocated_memory] = memory_consumption(
          [&, vc = value_column]() { return encoder->encode(DataType::Int, vc); });

      auto benchmark_state = benchmark_decompression_with_iterable(*encoded_column);

      auto results = benchmark_state.results();
      auto num_iterations = benchmark_state.num_iterations();

      _result_sets.push_back({encoding_spec, num_iterations, allocated_memory, std::move(results)});
    }

    _create_report();
  }

 private:
  std::unique_ptr<BenchmarkMemoryResource> _memory_resource;

  struct MeasurementResultSet{
    ColumnEncodingSpec encoding_spec;
    size_t num_iterations;
    size_t allocated_memory;
    std::vector<Duration> results;
  };

  std::vector<MeasurementResultSet> _result_sets;
};

}  // namespace opossum


int main(int argc, char const *argv[])
{
  auto benchmark = opossum::SingleDistributionBenchmark{};
  benchmark.run();

  return 0;
}
