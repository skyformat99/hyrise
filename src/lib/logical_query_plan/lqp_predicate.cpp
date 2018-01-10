#include "lqp_predicate.hpp"

#include "utils/assert.hpp"

namespace opossum {

LQPPredicate::LQPPredicate(const LQPColumnOrigin& column_origin,
             const ScanType scan_type,
             const AllParameterVariant& value,
             const std::optional<AllTypeVariant>& value2): column_origin(column_origin), scan_type(scan_type), value(value), value2(value2) {
  Assert(!is_column_id(value), "There shouldn't be a ColumnID in an LQP");
}

bool LQPPredicate::is_column_predicate() const {
  return is_lqp_column_origin(value);
}

bool LQPPredicate::is_value_predicate() const {
  return !is_column_predicate();
}

}  // namespace opossum