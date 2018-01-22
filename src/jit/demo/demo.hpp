#pragma once

#include <string>

#include "../../lib/operators/jit_operator/value/types.hpp"

namespace demo_1 {

class Base {
 public:
  virtual uint64_t foo() const;
  uint64_t bar() const;
};

class Impl : public Base {
 public:
  explicit Impl(const std::string& str);
  uint64_t foo() const final;
  std::string baz() const;

 private:
  const std::string _str;
};

}  // namespace demo_1

namespace demo_2 {

class ValueReader {
 public:
  using Ptr = std::shared_ptr<const ValueReader>;

  virtual opossum::JitVariant get_value(int32_t ctx) const = 0;
};

template <typename T>
class ConstantReader : public ValueReader {
 public:
  explicit ConstantReader(const T& val);
  opossum::JitVariant get_value(int32_t ctx) const final;

 private:
  const T _val;
};

class ContextReader : public ValueReader {
 public:
  opossum::JitVariant get_value(int32_t ctx) const final;
};

class NullReader : public ValueReader {
 public:
  opossum::JitVariant get_value(int32_t ctx) const final;
};

class ModuloReader : public ValueReader {
 public:
  opossum::JitVariant get_value(int32_t ctx) const final;
};

class CompareOperator {
 public:
  CompareOperator(const ValueReader::Ptr& left, const ValueReader::Ptr& right);
  bool execute(int32_t ctx) const;

 private:
  const ValueReader::Ptr _left, _right;
};

class SumOperator {
 public:
  void add_value(const ValueReader::Ptr& value);
  double execute(int32_t ctx) const;

 private:
  std::vector<ValueReader::Ptr> _values;
};

}  // namespace demo_2
