#include "module.hpp"

#include <queue>
#include <sstream>
#include <stdexcept>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/IRMover.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include "utils/llvm_utils.hpp"

namespace opossum {

Module::Module(const std::string& root_function_name)
    : _repository{IRRepository::get()},
      _module{std::make_unique<llvm::Module>(root_function_name, *_repository.llvm_context())},
      _compiler{_repository.llvm_context()} {
  auto root_function = _repository.get_function(root_function_name);
  DebugAssert(root_function, "Root function could not be found in repository.");
  _root_function = _clone_function(*root_function, "_");
}

void Module::specialize(const RuntimePointer::Ptr& runtime_this) {
  // add runtime value for this pointer
  _runtime_values[&*_root_function->arg_begin()] = runtime_this;

  std::queue<llvm::CallSite> call_sites;
  _visit<llvm::CallInst>([&](llvm::CallInst& inst) { call_sites.push(llvm::CallSite(&inst)); });
  _visit<llvm::InvokeInst>([&](llvm::InvokeInst& inst) { call_sites.push(llvm::CallSite(&inst)); });

  while (!call_sites.empty()) {
    auto& call_site = call_sites.front();

    if (call_site.isIndirectCall()) {
      // attempt to resolve virtual calls
      auto called_value = call_site.getCalledValue();
      auto called_runtime_value = std::static_pointer_cast<const KnownRuntimePointer>(_get_runtime_value(called_value));
      if (called_runtime_value->is_known()) {
        auto vtable_index = called_runtime_value->up().total_offset() / _module->getDataLayout().getPointerSize();
        auto instance = reinterpret_cast<RTTIHelper*>(called_runtime_value->up().up().base().address());
        auto class_name = typeid(*instance).name();
        if (auto repo_function = _repository.get_vtable_entry(class_name, vtable_index)) {
          auto cloned_function = _clone_function(*repo_function);

          auto arg = call_site.arg_begin();
          auto cloned_arg = cloned_function->arg_begin();
          for (; arg != call_site.arg_end() && cloned_arg != cloned_function->arg_end(); ++arg, ++cloned_arg) {
            if (arg->get()->getType() != cloned_arg->getType()) {
              *arg = new llvm::BitCastInst(arg->get(), cloned_arg->getType(), "", call_site.getInstruction());
            }
          }

          call_site.setCalledFunction(cloned_function);
        }
      }
    } else if (!call_site.getCalledFunction()->isDeclaration()) {
      auto cloned_function = _clone_function(*call_site.getCalledFunction());
      call_site.setCalledFunction(cloned_function);
    }

    llvm::InlineFunctionInfo info;
    if (llvm::InlineFunction(call_site, info)) {
      call_site.getCalledFunction()->eraseFromParent();
      for (auto& new_call_site : info.InlinedCallSites) {
        call_sites.push(new_call_site);
      }
    }

    call_sites.pop();
  }

  _replace_loads_with_runtime_values();
}

void Module::_compile_impl() {
  // note: strangely, llvm::verifyModule returns false for valid modules
  Assert(!llvm::verifyModule(*_module, &llvm::dbgs()), "Module is invalid.");

  _optimize();
  _compiler.add_module(std::move(_module));
}

void Module::_optimize() {
  auto before_path = llvm_utils::temp_file("ll");
  auto after_path = llvm_utils::temp_file("ll");
  auto remarks_path = llvm_utils::temp_file("yml");

  std::cout << "Running optimization" << std::endl
            << "  before:  " << before_path.string() << std::endl
            << "  after:   " << after_path.string() << std::endl
            << "  remarks: " << remarks_path.string() << std::endl;

  llvm_utils::module_to_file(before_path, *_module);

  std::ostringstream command;
  command << "opt-5.0 -O3 -S -o " << after_path << " " << before_path << " -pass-remarks-output=" << remarks_path;
  system(command.str().c_str());

  auto optimized_module = llvm_utils::module_from_file(after_path, _module->getContext());
  _compiler.add_module(std::move(optimized_module));
}

void Module::_replace_loads_with_runtime_values() {
  _visit<llvm::LoadInst>([&](llvm::LoadInst& inst) {
    auto runtime_pointer =
        std::dynamic_pointer_cast<const KnownRuntimePointer>(_get_runtime_value(inst.getPointerOperand()));
    if (!runtime_pointer) {
      return;
    }
    auto address = runtime_pointer->address();

    if (inst.getType()->isIntegerTy()) {
      auto bit_width = inst.getType()->getIntegerBitWidth();
      auto mask = bit_width == 64
                      ? 0xffffffffffffffff
                      : (static_cast<uint64_t>(1) << inst.getType()->getIntegerBitWidth()) - 1;
      auto value = *reinterpret_cast<uint64_t*>(address) & mask;
      inst.replaceAllUsesWith(llvm::ConstantInt::get(inst.getType(), value));
    } else if (inst.getType()->isPointerTy()) {
      auto int_address = llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(*_repository.llvm_context()),
                                                *reinterpret_cast<uint64_t*>(address));
      inst.replaceAllUsesWith(new llvm::IntToPtrInst(int_address, inst.getType(), "", &inst));
    }
  });
}

llvm::Function* Module::_create_function_declaration(const llvm::Function& function, const std::string& suffix) {
  auto declaration = llvm::Function::Create(llvm::cast<llvm::FunctionType>(function.getValueType()),
                                            function.getLinkage(), function.getName() + suffix, _module.get());
  declaration->copyAttributesFrom(&function);
  return declaration;
}

llvm::Function* Module::_clone_function(const llvm::Function& function, const std::string& suffix) {
  auto cloned_function = _create_function_declaration(function, suffix);

  // map personality function
  if (function.hasPersonalityFn()) {
    _visit<llvm::Function>(*function.getPersonalityFn(), [&](auto& fn) {
      if (!_llvm_value_map.count(&fn)) {
        _llvm_value_map[&fn] = _create_function_declaration(fn);
      }
    });
  }

  // map functions called
  _visit<const llvm::Function>(function, [&](const auto& fn) {
    if (fn.isDeclaration() && !_llvm_value_map.count(&fn)) {
      _llvm_value_map[&fn] = _create_function_declaration(fn);
    }
  });

  // map global variables
  _visit<const llvm::GlobalVariable>(function, [&](auto& global) { _llvm_value_map[&global] = _clone_global(global); });

  // map function args
  auto arg = function.arg_begin();
  auto cloned_arg = cloned_function->arg_begin();
  for (; arg != function.arg_end() && cloned_arg != cloned_function->arg_end(); ++arg, ++cloned_arg) {
    cloned_arg->setName(arg->getName());
    _llvm_value_map[arg] = cloned_arg;
  }

  llvm::SmallVector<llvm::ReturnInst*, 8> returns;
  llvm::CloneFunctionInto(cloned_function, &function, _llvm_value_map, true, returns);

  if (function.hasPersonalityFn()) {
    cloned_function->setPersonalityFn(llvm::MapValue(function.getPersonalityFn(), _llvm_value_map));
  }

  return cloned_function;
}

llvm::GlobalVariable* Module::_clone_global(const llvm::GlobalVariable& global) {
  auto cloned_global = new llvm::GlobalVariable(*_module, global.getValueType(), global.isConstant(),
                                                global.getLinkage(), nullptr, global.getName(), nullptr,
                                                global.getThreadLocalMode(), global.getType()->getAddressSpace());

  cloned_global->copyAttributesFrom(&global);

  if (!global.isDeclaration()) {
    if (global.hasInitializer()) {
      cloned_global->setInitializer(llvm::MapValue(global.getInitializer(), _llvm_value_map));
    }

    llvm::SmallVector<std::pair<uint32_t, llvm::MDNode*>, 1> metadata_nodes;
    global.getAllMetadata(metadata_nodes);
    for (auto& metadata_node : metadata_nodes) {
      cloned_global->addMetadata(metadata_node.first,
                                 *MapMetadata(metadata_node.second, _llvm_value_map, llvm::RF_MoveDistinctMDs));
    }
  }

  return cloned_global;
}

RuntimePointer::Ptr& Module::_get_runtime_value(const llvm::Value* value) {
  // try serving from cache
  if (_runtime_values.count(value)) {
    return _runtime_values[value];
  }

  if (auto load_inst = llvm::dyn_cast<llvm::LoadInst>(value)) {
    if (load_inst->getType()->isPointerTy()) {
      if (auto base = std::dynamic_pointer_cast<const KnownRuntimePointer>(
              _get_runtime_value(load_inst->getPointerOperand()))) {
        _runtime_values[value] = std::make_shared<DereferencedRuntimePointer>(base);
      }
    }
  } else if (auto gep_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(value)) {
    llvm::APInt offset(64, 0);
    if (gep_inst->accumulateConstantOffset(_module->getDataLayout(), offset)) {
      if (auto base =
              std::dynamic_pointer_cast<const KnownRuntimePointer>(_get_runtime_value(gep_inst->getPointerOperand()))) {
        _runtime_values[value] = std::make_shared<OffsetRuntimePointer>(base, offset.getLimitedValue());
      }
    }
  } else if (auto bitcast_inst = llvm::dyn_cast<llvm::BitCastInst>(value)) {
    if (auto base =
            std::dynamic_pointer_cast<const KnownRuntimePointer>(_get_runtime_value(bitcast_inst->getOperand(0)))) {
      _runtime_values[value] = std::make_shared<OffsetRuntimePointer>(base, 0L);
    }
  }

  if (!_runtime_values.count(value)) {
    _runtime_values[value] = std::make_shared<RuntimePointer>();
  }

  return _runtime_values[value];
}

template <typename T, typename U>
void Module::_visit(U& element, std::function<void(T&)> fn) {
  // clang-format off
  if constexpr(std::is_same_v<std::remove_const_t<U>, llvm::Module>) {
    for (auto& function : element) {
      _visit(function, fn);
    }
  } else if constexpr(std::is_same_v<std::remove_const_t<U>, llvm::Function>) {
    for (auto& block : element) {
      _visit(block, fn);
    }
  } else if constexpr(std::is_same_v<std::remove_const_t<U>, llvm::BasicBlock>) {
    for (auto& inst : element) {
      _visit(inst, fn);
    }
  } else if constexpr(std::is_same_v<std::remove_const_t<U>, llvm::Instruction>) {
    if constexpr(std::is_base_of_v<llvm::Instruction, T>) {
      if (auto inst = llvm::dyn_cast<T>(&element)) {
        fn(*inst);
      }
    } else {
      for (auto& op : element.operands()) {
        _visit(*op.get(), fn);
      }
    }
  } else if constexpr(std::is_same_v<std::remove_const_t<U>, llvm::ConstantExpr>) {
    for (auto& op : element.operands()) {
      _visit(*op.get(), fn);
    }
  } else {
    if (auto op = llvm::dyn_cast<T>(&element)) {
      fn(*op);
    } else if (auto const_expr = llvm::dyn_cast<llvm::ConstantExpr>(&element)) {
      _visit(*const_expr, fn);
    }
  }
  // clang-format on
}

template <typename T>
void Module::_visit(std::function<void(T&)> fn) {
  _visit(*_root_function, fn);
}

}  // namespace opossum
