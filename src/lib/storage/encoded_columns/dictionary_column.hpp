#pragma once

#include <memory>
#include <string>

#include "base_dictionary_column.hpp"
#include "types.hpp"

namespace opossum {

class BaseZeroSuppressionVector;

/**
 * @brief Column implementing dictionary encoding
 *
 * This is the updated dictionary column, which uses the
 * new zero suppression encodings for its attribute vector.
 * Eventually the old implementation is going to be completely
 * replaced with new one.
 */
template <typename T>
class DictionaryColumn : public BaseDictionaryColumn {
 public:
  explicit DictionaryColumn(const std::shared_ptr<const pmr_vector<T>>& dictionary,
                            const std::shared_ptr<const BaseZeroSuppressionVector>& attribute_vector, const ValueID null_value_id);

  // returns an underlying dictionary
  std::shared_ptr<const pmr_vector<T>> dictionary() const;

  /**
   * @defgroup BaseColumn interface
   * @{
   */

  const AllTypeVariant operator[](const ChunkOffset chunk_offset) const final;

  size_t size() const final;

  void write_string_representation(std::string& row_string, const ChunkOffset chunk_offset) const final;

  void copy_value_to_value_column(BaseColumn& value_column, ChunkOffset chunk_offset) const final;

  std::shared_ptr<BaseColumn> copy_using_allocator(const PolymorphicAllocator<size_t>& alloc) const final;

  /**@}*/

  /**
   * @defgroup BaseDictionaryColumn interface
   * @{
   */

  ValueID lower_bound(const AllTypeVariant& value) const final;
  ValueID upper_bound(const AllTypeVariant& value) const final;

  size_t unique_values_count() const final;

  std::shared_ptr<const BaseZeroSuppressionVector> attribute_vector() const final;

  const ValueID null_value_id() const final;

  /**@}*/

 protected:
  const std::shared_ptr<const pmr_vector<T>> _dictionary;
  const std::shared_ptr<const BaseZeroSuppressionVector> _attribute_vector;
  const ValueID _null_value_id;
};

struct DictionaryColumnInfo {
  template <typename T>
  using ColumnTemplate = DictionaryColumn<T>;
};

}  // namespace opossum