#include "demo.hpp"

#include <iostream>

namespace demo {

uint64_t Base::foo() const { return 1; }
uint64_t Base::bar() const { return 2 * foo(); }

Impl::Impl(const std::string& str) : _str{str} {}
uint64_t Impl::foo() const { return baz().size(); }
__attribute__((noinline)) std::string Impl::baz() const { return _str; }

}  // namespace demo
