#pragma once

#include <functional>

#include <boost/variant.hpp>

namespace opossum {

struct Jit3VBool {
  const bool is_known;
  const bool value;

  operator bool() {
    return is_known ? value : throw std::logic_error("accessing unknown bool");
  }
};

struct Jit3VComparison {
 public:
  Jit3VComparison(): is_known{false}, result{0} {}
  explicit Jit3VComparison(const int32_t res): is_known{true}, result{res} {}

  Jit3VBool to_bool(const std::function<bool(int32_t)>& fn) const { return {is_known, fn(result)}; }

  const bool is_known;
  const int32_t result;
};

template <typename T>
struct Get : public boost::static_visitor<T> {
  template <typename U>
  auto operator()(const U& value) const -> decltype(static_cast<T>(value)) {
    return static_cast<T>(value);
  }

  template <typename... Ts>
  T operator()(const Ts...) const {
    throw std::invalid_argument("can't get");
  }
};

// we need to use inheritance here, so we can implement our own comparison operators
class JitVariant : public boost::variant<boost::blank, int32_t, int64_t, float, double, std::string> {
 public:
  template <typename T>
  T get() { return boost::apply_visitor(Get<T>(), *this); }
};

struct Negate : public boost::static_visitor<JitVariant> {
  JitVariant operator()(const boost::blank&) const { return JitVariant(); }

  template <typename T>
  auto operator()(const T& value) const -> decltype(JitVariant{-value}) {
    return JitVariant{-value};
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't negate");
  }
};

struct Add : public boost::static_visitor<JitVariant> {
  template <typename T, typename U, typename = std::enable_if_t<std::is_same_v<T, boost::blank> || std::is_same_v<U, boost::blank>, void>>
  JitVariant operator()(const T&, const U&) const { return JitVariant(); }

  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant{lhs + rhs}) {
    return JitVariant{lhs + rhs};
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't add");
  }
};

struct Subtract : public boost::static_visitor<JitVariant> {
  template <typename T, typename U, typename = std::enable_if_t<std::is_same_v<T, boost::blank> || std::is_same_v<U, boost::blank>, void>>
  JitVariant operator()(const T&, const U&) const { return JitVariant(); }

  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant{lhs - rhs}) {
    return JitVariant{lhs - rhs};
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't subtract");
  }
};

struct Multiply : public boost::static_visitor<JitVariant> {
  template <typename T, typename U, typename = std::enable_if_t<std::is_same_v<T, boost::blank> || std::is_same_v<U, boost::blank>, void>>
  JitVariant operator()(const T&, const U&) const { return JitVariant(); }

  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant{lhs * rhs}) {
    return JitVariant{lhs * rhs};
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't multiply");
  }
};

struct Divide : public boost::static_visitor<JitVariant> {
  template <typename T, typename U, typename = std::enable_if_t<std::is_same_v<T, boost::blank> || std::is_same_v<U, boost::blank>, void>>
  JitVariant operator()(const T&, const U&) const { return JitVariant(); }

  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant{lhs / rhs}) {
    return JitVariant{lhs / rhs};
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't divide");
  }
};

struct Compare : public boost::static_visitor<Jit3VComparison> {
  template <typename T, typename U, typename = std::enable_if_t<std::is_same_v<T, boost::blank> || std::is_same_v<U, boost::blank>, void>>
  Jit3VComparison operator()(const T&, const U&) const { return Jit3VComparison(); }

  Jit3VComparison operator()(const std::string& lhs, const std::string& rhs) const {
    return Jit3VComparison(lhs.compare(rhs));
  }

  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(lhs - rhs, Jit3VComparison()) {
    decltype(lhs - rhs) zero = 0;
    return Jit3VComparison((zero < (lhs - rhs)) - ((lhs - rhs) < zero));
  }

  template <typename... Ts>
  Jit3VComparison operator()(const Ts...) const {
    throw std::invalid_argument("can't compare");
  }
};

}  // namespace opossum

inline opossum::Jit3VBool operator==(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result == 0; });
}

inline opossum::Jit3VBool operator!=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result != 0; });
}

inline opossum::Jit3VBool operator<(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result < 0; });
}

inline opossum::Jit3VBool operator<=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result <= 0; });
}

inline opossum::Jit3VBool operator>(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result > 0; });
}

inline opossum::Jit3VBool operator>=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs).to_bool([](const auto result) { return result >= 0; });
}

inline opossum::JitVariant operator-(const opossum::JitVariant& val) {
  return boost::apply_visitor(opossum::Negate(), val);
}

inline opossum::JitVariant operator+(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Add(), lhs, rhs);
}

inline opossum::JitVariant operator-(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Subtract(), lhs, rhs);
}

inline opossum::JitVariant operator*(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Multiply(), lhs, rhs);
}

inline opossum::JitVariant operator/(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Divide(), lhs, rhs);
}
