#include <iostream>

#include "../lib/operators/jit_operator/specialization/module.hpp"
#include "demo/demo.hpp"

int main(int argc, char* argv[]) {
  std::cout << "== Jit Specialization Test ==" << std::endl << std::endl;

  const demo_1::Impl instance{"acht"};
  opossum::Module module("_ZNK6demo_14Base3barEv");
  module.specialize(std::make_shared<opossum::ConstantRuntimePointer>(&instance));
  const auto bar_fn = module.compile<int32_t(const demo_1::Base*)>();

  std::cout << "actual:   " << bar_fn(&instance) << std::endl;
  std::cout << "expected: " << instance.bar() << std::endl;
}
