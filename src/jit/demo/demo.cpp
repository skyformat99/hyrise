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

opossum::JitVariant ContextReader::get_value(int32_t ctx) const { return opossum::JitVariant{ctx}; }

template class ConstantReader<int32_t>;
template class ConstantReader<int64_t>;
template class ConstantReader<float>;
template class ConstantReader<double>;
template class ConstantReader<std::string>;

Operator::Operator(const ValueReader::Ptr& left, const ValueReader::Ptr& right) : _left{left}, _right{right} {}

bool Operator::execute(int32_t ctx) const { return _left->get_value(ctx) == _right->get_value(ctx); }

}  // namespace demo_2

