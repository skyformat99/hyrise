#pragma once

#include "all_parameter_variant.hpp"
#include "all_type_variant.hpp"
#include "lqp_column_origin.hpp"
#include "types.hpp"

namespace opossum {

struct LQPPredicate final {
  LQPPredicate(const LQPColumnOrigin& column_origin,
                      const ScanType scan_type,
                      const AllParameterVariant& value,
                      const std::optional<AllTypeVariant>& value2 = std::nullopt);

  bool is_column_predicate() const;
  bool is_value_predicate() const;

  const LQPColumnOrigin column_origin;
  const ScanType scan_type;
  const AllParameterVariant value;
  const std::optional<const AllTypeVariant> value2;
};

}