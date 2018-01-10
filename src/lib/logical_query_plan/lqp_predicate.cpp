#include "lqp_predicate.hpp"

namespace opossum {

LQPPredicate::LQPPredicate(const LQPColumnOrigin& column_origin,
             const ScanType scan_type,
             const AllParameterVariant& value,
             const std::optional<AllTypeVariant>& value2 = std::nullopt): column_origin(column_origin), scan_type(scan_type), value(value), value2(value2) {

}

}  // namespace opossum