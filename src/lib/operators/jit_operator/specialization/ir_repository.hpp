#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/Module.h>

#include "types.hpp"

namespace opossum {

// operators
extern char jit_aggregate;
extern size_t jit_aggregate_size;

extern char jit_binary_compute;
extern size_t jit_binary_compute_size;

extern char jit_get_table;
extern size_t jit_get_table_size;

extern char jit_table_scan;
extern size_t jit_table_scan_size;

extern char jit_print;
extern size_t jit_print_size;

// value access
extern char types;
extern size_t types_size;

extern char value_reader;
extern size_t value_reader_size;

extern char value_writer;
extern size_t value_writer_size;

// demo - for debugging and testing only
extern char demo;
extern size_t demo_size;

// Singleton
class IRRepository : private Noncopyable {
 public:
  static IRRepository& get();

  const llvm::Function* get_function(const std::string& name) const;
  const llvm::Function* get_vtable_entry(const std::string& class_name, const size_t index) const;

  std::shared_ptr<llvm::LLVMContext> llvm_context() const;

 private:
  IRRepository();

  void _add_module(const std::string& str);
  void _dump(std::ostream& os) const;

  std::shared_ptr<llvm::LLVMContext> _llvm_context;
  std::vector<std::unique_ptr<const llvm::Module>> _modules;
  std::unordered_map<std::string, const llvm::Function*> _functions;
  std::unordered_map<std::string, std::vector<const llvm::Function*>> _vtables;

  const std::string vtable_prefix = "_ZTV";
};

}  // namespace opossum
