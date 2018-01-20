#pragma once

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Error.h>
#include <llvm/Target/TargetMachine.h>

#include "utils/error_utils.hpp"

namespace opossum {

class JitCompiler {
 protected:
  using ObjectLayer = llvm::orc::RTDyldObjectLinkingLayer;
  using CompileLayer = llvm::orc::IRCompileLayer<ObjectLayer, llvm::orc::SimpleCompiler>;
  using ModuleHandle = CompileLayer::ModuleHandleT;

 public:
  explicit JitCompiler(std::shared_ptr<llvm::LLVMContext> context);
  ~JitCompiler();

  ModuleHandle add_module(std::shared_ptr<llvm::Module> module);

  void remove_module(const ModuleHandle& handle);

  template <typename T>
  std::function<T> find_symbol(const std::string& name) {
    llvm::JITTargetAddress target_address =
        error_utils::handle_error(_compile_layer.findSymbol(_mangle(name), true).getAddress());

    if (!target_address) {
      llvm::errs() << "Symbol " << name << " could not be found.\n";
      llvm_unreachable("");
    }

    return reinterpret_cast<T*>(target_address);
  }

 private:
  std::string _mangle(const std::string& name) const;

  const std::shared_ptr<llvm::LLVMContext> _context;
  const std::unique_ptr<llvm::TargetMachine> _target_machine;
  const llvm::DataLayout _data_layout;
  ObjectLayer _object_layer;
  CompileLayer _compile_layer;
  llvm::orc::LocalCXXRuntimeOverrides _cxx_runtime_overrides;
  std::vector<ModuleHandle> _modules;
};

}  // namespace opossum
