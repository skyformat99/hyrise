#pragma once

#include <stack>

#include <llvm/IR/Module.h>

#include "ir_repository.hpp"
#include "jit_compiler.hpp"
#include "runtime_pointer.hpp"

namespace opossum {

class RTTIHelper {
 private:
  virtual void _() const {}
};

class Module {
 public:
  explicit Module(const std::string& root_function_name);

  void specialize(const RuntimePointer::Ptr& runtime_this);

  template <typename T>
  std::function<T> compile() {
    auto function_name = _module->getName().str() + "_";
    _compile_impl();
    return _compiler.find_symbol<T>(function_name);
  }

 private:
  void _compile_impl();

  void _optimize();

  void _replace_loads_with_runtime_values();

  llvm::Function* _create_function_declaration(const llvm::Function& function, const std::string& suffix = "");

  llvm::Function* _clone_function(const llvm::Function& function, const std::string& suffix = "");

  llvm::GlobalVariable* _clone_global(const llvm::GlobalVariable& global);

  RuntimePointer::Ptr& _get_runtime_value(const llvm::Value* value);

  template <typename T, typename U>
  void _visit(U& function, std::function<void(T&)> fn);

  template <typename T>
  void _visit(std::function<void(T&)> fn);

  // void _process_function(llvm::Function* function, const std::vector<RuntimeValue::Ptr>& runtime_arguments);

  // llvm::Function* _clone_from_repo(llvm::Function* function);

  // llvm::Function* _get_or_create_decleration(llvm::Function* function);

  // template <typename T>
  // void _replace_virtual_calls(llvm::Function* function);

  // void _replace_loads_with_runtime_values(llvm::Function* function);

  // template <typename T>
  // void _handle_direct_calls(llvm::Function* function);

  // template <typename T>
  // void _inline_functions();

  // template <typename T, typename C>
  // void _for_each(const C& container, std::function<void(T* inst)> func);

  const IRRepository& _repository;
  std::unique_ptr<llvm::Module> _module;
  JitCompiler _compiler;

  llvm::Function* _root_function;
  llvm::ValueToValueMapTy _llvm_value_map;
  std::unordered_map<const llvm::Value*, RuntimePointer::Ptr> _runtime_values;
};

}  // namespace opossum
