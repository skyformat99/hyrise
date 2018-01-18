#include <bitset>
#include <iostream>
#include <memory>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "storage/zero_suppression/resolve_zs_vector_type.hpp"
#include "storage/zero_suppression/utils.hpp"
#include "storage/zero_suppression/vectors.hpp"
#include "storage/zero_suppression/zs_type.hpp"

#include "types.hpp"

namespace opossum {

namespace {

// Used for debugging purposes
[[maybe_unused]] void print_encoded_vector(const SimdBp128Vector& vector) {
  for (auto _128_bit : vector.data()) {
    for (auto _32_bit : _128_bit.data) {
      std::cout << std::bitset<32>{_32_bit} << "|";
    }
    std::cout << std::endl;
  }
}

}  // namespace

class ZeroSuppressionTest : public BaseTest, public ::testing::WithParamInterface<ZsType> {
 protected:
  void SetUp() override {}

  auto min() { return 1'024; }

  auto max() { return 34'624; }

  pmr_vector<uint32_t> generate_sequence(size_t count, uint32_t increment) {
    auto sequence = pmr_vector<uint32_t>(count);
    auto value = min();
    for (auto& elem : sequence) {
      elem = value;

      value += increment;
      if (value > max()) value = min();
    }

    return sequence;
  }

  std::unique_ptr<BaseZeroSuppressionVector> encode(const pmr_vector<uint32_t>& vector) {
    const auto zs_type = GetParam();

    auto encoded_vector = encode_by_zs_type(zs_type, vector, {}, {max()});
    EXPECT_EQ(encoded_vector->size(), vector.size());

    return encoded_vector;
  }

  template <typename ZeroSuppressionVectorT>
  void compare_using_iterator(const ZeroSuppressionVectorT& encoded_sequence,
                              const pmr_vector<uint32_t>& expected_values) {
    auto expected_it = expected_values.cbegin();
    auto encoded_seq_it = encoded_sequence.cbegin();
    const auto encoded_seq_end = encoded_sequence.cend();
    for (; encoded_seq_it != encoded_seq_end; expected_it++, encoded_seq_it++) {
      EXPECT_EQ(*encoded_seq_it, *expected_it);
    }
  }
};

auto formatter = [](const ::testing::TestParamInfo<ZsType> info) {
  return std::to_string(static_cast<uint32_t>(info.param));
};

// As long as two implementation of dictionary encoding exist, this ensure to run the tests for both.
INSTANTIATE_TEST_CASE_P(ZsTypes, ZeroSuppressionTest,
                        ::testing::Values(ZsType::SimdBp128, ZsType::FixedSizeByteAligned), formatter);

TEST_P(ZeroSuppressionTest, DecodeIncreasingSequenceUsingIterators) {
  const auto sequence = this->generate_sequence(4'200, 8u);
  const auto encoded_sequence_base = this->encode(sequence);

  resolve_zs_vector_type(*encoded_sequence_base,
                         [&](auto& encoded_sequence) { compare_using_iterator(encoded_sequence, sequence); });
}

TEST_P(ZeroSuppressionTest, DecodeIncreasingSequenceUsingDecoder) {
  const auto sequence = this->generate_sequence(4'200, 8u);
  const auto encoded_sequence = this->encode(sequence);

  auto decoder = encoded_sequence->create_base_decoder();

  auto seq_it = sequence.cbegin();
  const auto seq_end = sequence.cend();
  auto index = 0u;
  for (; seq_it != seq_end; seq_it++, index++) {
    EXPECT_EQ(*seq_it, decoder->get(index));
  }
}

TEST_P(ZeroSuppressionTest, DecodeSequenceOfZerosUsingIterators) {
  const auto sequence = pmr_vector<uint32_t>(2'200, 0u);
  const auto encoded_sequence_base = this->encode(sequence);

  resolve_zs_vector_type(*encoded_sequence_base,
                         [&](auto& encoded_sequence) { compare_using_iterator(encoded_sequence, sequence); });
}

TEST_P(ZeroSuppressionTest, DecodeSequenceOfZerosUsingDecoder) {
  const auto sequence = pmr_vector<uint32_t>(2'200, 0u);
  const auto encoded_sequence = this->encode(sequence);

  auto decoder = encoded_sequence->create_base_decoder();

  auto seq_it = sequence.cbegin();
  const auto seq_end = sequence.cend();
  auto index = 0u;
  for (; seq_it != seq_end; seq_it++, index++) {
    EXPECT_EQ(*seq_it, decoder->get(index));
  }
}

TEST_P(ZeroSuppressionTest, DecodeSequenceOfZerosUsingDecodeMethod) {
  const auto sequence = pmr_vector<uint32_t>(2'200, 0u);
  const auto encoded_sequence = this->encode(sequence);

  auto decoded_sequence = encoded_sequence->decode();

  auto seq_it = sequence.cbegin();
  const auto seq_end = sequence.cend();
  auto decoded_seq_it = decoded_sequence.cbegin();
  for (; seq_it != seq_end; seq_it++, decoded_seq_it++) {
    EXPECT_EQ(*seq_it, *decoded_seq_it);
  }
}

}  // namespace opossum
