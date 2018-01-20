#pragma once

#include <boost/filesystem.hpp>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Regex.h>

#include "error_utils.hpp"

namespace opossum {

struct llvm_utils {
  static boost::filesystem::path temp_file(const std::string& extension) {
    return boost::filesystem::temp_directory_path() /
           boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%." + extension);
  }

  static void module_to_file(const boost::filesystem::path& path, const llvm::Module& module) {
    if (path.extension() == ".ll") {
      std::string content;
      llvm::raw_string_ostream sos(content);
      module.print(sos, nullptr, false, true);
      boost::filesystem::ofstream ofs(path);
      ofs << content;
    } else if (path.extension() == ".bc") {
      std::error_code error_code;
      llvm::raw_fd_ostream os(path.string(), error_code, llvm::sys::fs::F_None);
      llvm::WriteBitcodeToFile(&module, os);
      os.flush();
      error_utils::handle_error(error_code.value());
    } else {
      throw std::invalid_argument("invalid file extension for LLVM bitcode file");
    }
  }

  static std::unique_ptr<llvm::Module> module_from_file(const boost::filesystem::path& path,
                                                        llvm::LLVMContext& context) {
    llvm::SMDiagnostic error;
    auto module = llvm::parseIRFile(path.string(), error, context);
    error_utils::handle_error(error);
    return module;
  }

  static std::unique_ptr<llvm::Module> module_from_string(const std::string& str, llvm::LLVMContext& context) {
    llvm::SMDiagnostic error;
    auto buffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(str));
    auto module = llvm::parseIR(*buffer, error, context);
    error_utils::handle_error(error);
    return module;
  }
};

}  // namespace opossum
