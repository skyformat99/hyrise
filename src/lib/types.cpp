#include "types.hpp"

namespace opossum {

ScanType flip_scan_type(ScanType scan_type) {
  switch (scan_type) {
    case ScanType::Equals:
      return ScanType::Equals;
    case ScanType::NotEquals:
      return ScanType::NotEquals;
    case ScanType::LessThan:
      return ScanType::GreaterThanEquals;
    case ScanType::LessThanEquals:
      return ScanType::GreaterThan;
    case ScanType::GreaterThan:
      return ScanType::LessThanEquals;
    case ScanType::GreaterThanEquals:
      return ScanType::LessThan;
    case ScanType::Like:
      return ScanType::Like;
    case ScanType::NotLike:
      return ScanType::NotLike;
    case ScanType::IsNull:
      return ScanType::IsNull;
    case ScanType::IsNotNull:
      return ScanType::IsNotNull;
    default:
      Fail("Can't flip ScanType");
      return scan_type;  // Return something to make compilers happy;
  }
}

}  // namespace opossum;