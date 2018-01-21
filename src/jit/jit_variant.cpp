#include <iostream>

#include "../lib/operators/jit_operator/specialization/module.hpp"
#include "../lib/operators/jit_operator/value/types.hpp"
#include "demo/demo.hpp"

int main() {
  std::cout << "== Jit Variant Test ==" << std::endl << std::endl;

  const auto readerA = std::make_shared<demo_2::ConstantReader<int32_t>>(1);
  const auto readerB = std::make_shared<demo_2::ConstantReader<float>>(1.0);
  const auto readerC = std::make_shared<demo_2::ContextReader>();

  const demo_2::Operator instance(readerA, readerC);
  opossum::Module module("_ZNK6demo_28Operator7executeEi");
  module.specialize(std::make_shared<opossum::ConstantRuntimePointer>(&instance));
  const auto execute_fn = module.compile<bool(const demo_2::Operator*, int32_t)>();

  std::cout << "actual:   " << execute_fn(&instance, 2) << std::endl;
  std::cout << "expected: " << instance.execute(2) << std::endl;
}
