#pragma once

#include <string>

namespace demo {

class Base {
 public:
  virtual uint64_t foo() const;
  uint64_t bar() const;
};

class Impl : public Base {
 public:
  explicit Impl(const std::string& str);
  uint64_t foo() const final;
  std::string baz() const;

 private:
  const std::string _str;
};

}  // namespace demo
