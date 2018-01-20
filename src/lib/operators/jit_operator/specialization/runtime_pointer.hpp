#pragma once

#include <llvm/IR/Value.h>

namespace opossum {

class RuntimePointer {
 public:
  using Ptr = std::shared_ptr<const RuntimePointer>;

  virtual bool is_known() const { return false; }
};

class KnownRuntimePointer : public RuntimePointer {
 public:
  using Ptr = std::shared_ptr<const KnownRuntimePointer>;

  bool is_known() const final { return true; }

  virtual uint64_t address() const = 0;
  virtual uint64_t total_offset() const = 0;
  virtual const KnownRuntimePointer& base() const = 0;
  virtual const KnownRuntimePointer& up() const = 0;
};

class ConstantRuntimePointer : public KnownRuntimePointer {
 public:
  template <typename T>
  explicit ConstantRuntimePointer(const T* ptr) : _address{reinterpret_cast<uint64_t>(ptr)} {}
  template <typename T>
  explicit ConstantRuntimePointer(const std::shared_ptr<T>& ptr) : _address{reinterpret_cast<uint64_t>(ptr.get())} {}

  uint64_t address() const final { return _address; }
  uint64_t total_offset() const final { return 0L; }
  const KnownRuntimePointer& base() const final { return *this; }
  const KnownRuntimePointer& up() const final { return *this; }

 private:
  const uint64_t _address;
};

class OffsetRuntimePointer : public KnownRuntimePointer {
 public:
  OffsetRuntimePointer(const KnownRuntimePointer::Ptr& base, const uint64_t offset) : _base{base}, _offset{offset} {}

  uint64_t address() const final { return _base->address() + _offset; }
  uint64_t total_offset() const final { return _base->total_offset() + _offset; }
  const KnownRuntimePointer& base() const final { return _base->base(); }
  const KnownRuntimePointer& up() const final { return _base->up(); }

 private:
  const KnownRuntimePointer::Ptr _base;
  const uint64_t _offset;
};

class DereferencedRuntimePointer : public KnownRuntimePointer {
 public:
  explicit DereferencedRuntimePointer(const KnownRuntimePointer::Ptr& base) : _base{base} {}

  uint64_t address() const final { return *reinterpret_cast<uint64_t*>(_base->address()); }
  uint64_t total_offset() const final { return 0L; }
  const KnownRuntimePointer& base() const final { return *this; }
  const KnownRuntimePointer& up() const final { return *_base; }

 private:
  const KnownRuntimePointer::Ptr _base;
};

}  // namespace opossum