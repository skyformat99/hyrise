#include "demo.hpp"

#include <iostream>

namespace demo_1 {

uint64_t Base::foo() const { return 1; }

uint64_t Base::bar() const { return 2 * foo(); }

Impl::Impl(const std::string& str) : _str{str} {}

uint64_t Impl::foo() const { return baz().size(); }

__attribute__((noinline)) std::string Impl::baz() const { return _str; }

}  // namespace demo_1

namespace demo_2 {

template<typename T>
ConstantReader<T>::ConstantReader(const T& val) : _val{val} {}

template<typename T>
opossum::JitVariant ConstantReader<T>::get_value(int32_t ctx) const { return opossum::JitVariant{_val}; }

template class ConstantReader<int32_t>;
template class ConstantReader<int64_t>;
template class ConstantReader<float>;
template class ConstantReader<double>;
template class ConstantReader<std::string>;

opossum::JitVariant ContextReader::get_value(int32_t ctx) const { return opossum::JitVariant{ctx}; }

opossum::JitVariant NullReader::get_value(int32_t ctx) const { return opossum::JitVariant(); }

opossum::JitVariant ModuloReader::get_value(int32_t ctx) const { return (ctx % 2) ? opossum::JitVariant() : opossum::JitVariant{1}; }

CompareOperator::CompareOperator(const ValueReader::Ptr& left, const ValueReader::Ptr& right) : _left{left}, _right{right} {}

bool CompareOperator::execute(int32_t ctx) const { return _left->get_value(ctx) == _right->get_value(ctx); }

void SumOperator::add_value(const ValueReader::Ptr& value) { _values.push_back(value); }

double SumOperator::execute(int32_t ctx) const {
  opossum::JitVariant sum{0.0};
  for (uint32_t i = 0; i < _values.size(); ++i) {
    auto x = _values[i]->get_value(ctx);
    sum = sum + x;
  }
  return sum.get<double>();
}

}  // namespace demo_2
