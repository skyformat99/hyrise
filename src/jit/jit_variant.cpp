#include <iostream>

#include "../lib/operators/jit_operator/specialization/module.hpp"
#include "../lib/operators/jit_operator/value/types.hpp"
#include "demo/demo.hpp"

int main() {
  std::cout << "== Jit Variant Test ==" << std::endl << std::endl;

  const auto readerA = std::make_shared<demo_2::ConstantReader<int32_t>>(1);
  const auto readerB = std::make_shared<demo_2::ConstantReader<float>>(1.0);
  const auto readerC = std::make_shared<demo_2::ContextReader>();
  const auto readerD = std::make_shared<demo_2::NullReader>();
  const auto readerE = std::make_shared<demo_2::ModuloReader>();

  // CompareOperator
  const demo_2::CompareOperator compare_op(readerA, readerE);
  opossum::Module compare_module("_ZNK6demo_215CompareOperator7executeEi");
  compare_module.specialize(std::make_shared<opossum::ConstantRuntimePointer>(&compare_op));
  const auto compare_op_fn = compare_module.compile<bool(const demo_2::CompareOperator*, int32_t)>();

  std::cout << "actual:   " << compare_op_fn(&compare_op, 2) << std::endl;
  std::cout << "expected: " << compare_op.execute(2) << std::endl << std::endl;

  // SumOperator
  demo_2::SumOperator sum_op;
  sum_op.add_value(readerA);
  sum_op.add_value(readerB);
  sum_op.add_value(readerC);
  sum_op.add_value(readerE);
  opossum::Module sum_module("_ZNK6demo_211SumOperator7executeEi");
  sum_module.specialize(std::make_shared<opossum::ConstantRuntimePointer>(&sum_op));
  const auto sum_op_fn = sum_module.compile<double(const demo_2::SumOperator*, int32_t)>();

  std::cout << "actual:   " << sum_op_fn(&sum_op, 2) << std::endl;
  std::cout << "expected: " << sum_op.execute(2) << std::endl;
}
