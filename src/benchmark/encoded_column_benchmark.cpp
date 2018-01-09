#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <utility>
#include <numeric>

#include "storage/column_encoders/utils.hpp"
#include "storage/column_encoders/base_column_encoder.hpp"
#include "storage/encoded_columns/utils.hpp"
#include "storage/encoded_columns/column_encoding_type.hpp"
#include "storage/iterables/create_iterable_from_column.hpp"
#include "utils/numa_memory_resource.hpp"
#include "storage/encoded_columns/utils.hpp"

#include "benchmark_utilities/arithmetic_column_generator.hpp"

namespace opossum {

using Clock = std::chrono::high_resolution_clock;
using Duration = Clock::duration;
using TimePoint = Clock::time_point;

class Benchmark {
 public:
  enum class State { NotStarted, Running, Over };

 public:

  Benchmark(const size_t max_num_iterations, const Duration max_duration)
      : _max_num_iterations{max_num_iterations}, _max_duration{max_duration}, _state{State::NotStarted}, _num_iterations{0u} {}

  bool keep_running() {
    switch (_state) {
      case State::NotStarted:
        _init();
        return true;
      case State::Over:
        return false;
      default: {}
    }

    if (_num_iterations >= _max_num_iterations) {
      _end = Clock::now();
      _state = State::Over;
      return false;
    }

    _end = Clock::now();
    const auto duration = _end - _begin;
    if (duration >= _max_duration) {
      _state = State::Over;
      return false;
    }

    _num_iterations++;

    return true;
  }

  template <typename Functor>
  void measure(Functor functor) {
    _clear_cache();

    auto begin = Clock::now();
    functor();
    auto end = Clock::now();
    _results.push_back(end - begin);
  }

  std::vector<Duration> results() const { return _results; }
  size_t num_iterations() const { return _num_iterations; }

 private:
  void _init() {
    _state = State::Running;
    _num_iterations = 0u;
    _begin = Clock::now();
    _results = std::vector<Duration>();
    _results.reserve(_max_num_iterations);
  }

  void _clear_cache() {
    // TODO(mjendruk): Do some random reads to clear cache
  }

 private:
  const size_t _max_num_iterations;
  const Duration _max_duration;

  State _state;
  size_t _num_iterations;
  TimePoint _begin;
  TimePoint _end;

  std::vector<Duration> _results;
};

class ColumnCompressionBenchmark {
 public:
  ColumnCompressionBenchmark() {};

 private:
  auto _distribution_generators() {
    static const auto numa_node = 1;
    _memory_resource = std::make_unique<NUMAMemoryResource>(numa_node, "Numa node No. 1");
    auto alloc = PolymorphicAllocator<size_t>{_memory_resource.get()};

    auto generator = benchmark_utilities::ArithmeticColumnGenerator<int32_t>{alloc};

    using ValueColumnPtr = std::shared_ptr<ValueColumn<int32_t>>;
    auto dist_generators = std::vector<std::pair<std::string, std::function<ValueColumnPtr()>>>{
      { "Uniform from 0 to 4.000", [generator]() { return generator.uniformly_distributed_column(0, 10); }}};

    return dist_generators;
  }

  std::vector<EncodingType> _encoding_types() {
    return std::vector<EncodingType>{ EncodingType::DeprecatedDictionary, EncodingType::Dictionary, EncodingType::RunLength };
  }

  void _output_results() {
    for (const auto& result_set : _result_sets) {
      std::cout << "Distribution: " << result_set.distribution << std::endl;
      std::cout << "Encoding type: " << static_cast<uint32_t>(result_set.encoding_type) << std::endl;
      std::cout << "Num iterations: " << result_set.num_iterations << std::endl;

      const auto sum = std::accumulate(result_set.results.cbegin(), result_set.results.cend(), Clock::duration{});
      const auto average = sum / result_set.results.size();

      std::cout << "ms: " << std::chrono::duration_cast<std::chrono::milliseconds>(average).count() << std::endl;
    }
  }

 public:
  void run() {
    static const auto max_num_iterations = 1000u;
    static const auto max_duration = std::chrono::seconds{10};

    for (auto& [name, generator] : _distribution_generators()) {
      auto value_column = generator();

      auto benchmark = Benchmark{max_num_iterations, max_duration};
      while (benchmark.keep_running()) {
        benchmark.measure([&]() {
          auto iterable = create_iterable_from_column(*value_column);

          auto sum = 0;
          iterable.for_each([&](auto value) {
            sum += value.value();
          });
        });
      }

      auto results = benchmark.results();
      auto num_iterations = benchmark.num_iterations();
      _result_sets.push_back({name, EncodingType::Invalid, num_iterations, std::move(results)});

      for (auto encoding_type : _encoding_types()) {

        auto encoder = create_encoder(encoding_type);
        auto encoded_column = encoder->encode(DataType::Int, value_column);

        auto benchmark = Benchmark{max_num_iterations, max_duration};

        resolve_encoded_column_type<int32_t>(*encoded_column, [&](auto& typed_column) {
          while (benchmark.keep_running()) {
            benchmark.measure([&]() {
              auto iterable = create_iterable_from_column(typed_column);

              auto sum = 0;
              iterable.for_each([&](auto value) {
                sum += value.value();
              });
            });
          }
        });

        auto results = benchmark.results();
        auto num_iterations = benchmark.num_iterations();

        _result_sets.push_back({name, encoding_type, num_iterations, std::move(results)});
      }
    }

    _output_results();
  }

 private:
  std::unique_ptr<NUMAMemoryResource> _memory_resource;

  struct MeasurementResultSet{
    std::string distribution;
    EncodingType encoding_type;
    size_t num_iterations;
    std::vector<Duration> results;
  };

  std::vector<MeasurementResultSet> _result_sets;
};

}  // namespace opossum


int main(int argc, char const *argv[])
{
  auto benchmark = opossum::ColumnCompressionBenchmark{};
  benchmark.run();

  return 0;
}
