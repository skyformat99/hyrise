#pragma once

#include <boost/variant.hpp>

namespace opossum {

// we need to use inheritance here, so we can implement our own comparison operators
class JitVariant : public boost::variant<int32_t, int64_t, float, double, std::string> {};

struct Negate : public boost::static_visitor<JitVariant> {
  template <typename T>
  auto operator()(const T& val) const -> decltype(JitVariant(-val)) {
    return -val;
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't negate");
  }
};

struct Add : public boost::static_visitor<JitVariant> {
  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant(lhs + rhs)) {
    return lhs + rhs;
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't add");
  }
};

struct Subtract : public boost::static_visitor<JitVariant> {
  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant(lhs - rhs)) {
    return lhs - rhs;
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't subtract");
  }
};

struct Multiply : public boost::static_visitor<JitVariant> {
  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant(lhs * rhs)) {
    return lhs * rhs;
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't multiply");
  }
};

struct Divide : public boost::static_visitor<JitVariant> {
  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(JitVariant(lhs / rhs)) {
    return lhs / rhs;
  }

  template <typename... Ts>
  JitVariant operator()(const Ts...) const {
    throw std::invalid_argument("can't divide");
  }
};

struct Compare : public boost::static_visitor<int32_t> {
  template <typename T, typename U>
  auto operator()(const T& lhs, const U& rhs) const -> decltype(lhs - rhs, int32_t()) {
    decltype(lhs - rhs) zero = 0;
    return (zero < (lhs - rhs)) - ((lhs - rhs) < zero);
  }

  template <typename... Ts>
  int32_t operator()(const Ts...) const {
    throw std::invalid_argument("can't compare");
  }
};

}  // namespace opossum

inline bool operator==(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) == 0;
}

inline bool operator!=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) != 0;
}

inline bool operator<(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) < 0;
}

inline bool operator<=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) <= 0;
}

inline bool operator>(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) > 0;
}

inline bool operator>=(const opossum::JitVariant& lhs, const opossum::JitVariant& rhs) {
  return boost::apply_visitor(opossum::Compare(), lhs, rhs) >= 0;
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
