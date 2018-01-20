#include <iostream>

#include "../lib/operators/jit_operator/specialization/module.hpp"
#include "demo/demo.hpp"

int main(int argc, char* argv[]) {
  demo::Impl instance{"acht"};
  opossum::Module module("_ZNK4demo4Base3barEv");
  module.specialize(std::make_shared<opossum::ConstantRuntimePointer>(&instance));
  auto bar_fn = module.compile<int(demo::Base*)>();

  std::cout << "actual:   " << bar_fn(&instance) << std::endl;
  std::cout << "expected: " << instance.bar() << std::endl;
}
