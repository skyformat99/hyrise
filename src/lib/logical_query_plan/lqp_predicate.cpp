#include "lqp_predicate.hpp"

#include "utils/assert.hpp"

#include "constant_mappings.hpp"

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

bool LQPPredicate::operator==(const LQPPredicate& rhs) const {
  const auto equals = [&](const LQPPredicate& rhs) {
    return column_origin == rhs.column_origin && scan_type == rhs.scan_type && value == rhs.value && value2 == rhs.value2;
  };

  if (equals(rhs)) return true;

  if (rhs.scan_type != ScanType::Between && is_lqp_column_origin(rhs.value)) {
    return equals({boost::get<LQPColumnOrigin>(rhs.value), flip_scan_type(rhs.scan_type), rhs.column_origin});
  }

  return false;
}

void LQPPredicate::print(std::ostream& stream) const {
  stream << "{";
  stream << column_origin.description() << " ";
  stream << scan_type_to_string.left.at(scan_type) << " ";
  stream << value;

  if (value2) {
    stream << " AND " << *value2;
  }
  stream << "}";
}

}  // namespace opossum