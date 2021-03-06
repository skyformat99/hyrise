#include "import_binary.hpp"

#include <boost/hana/for_each.hpp>

#include <cstdint>
#include <fstream>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "constant_mappings.hpp"
#include "import_export/binary.hpp"
#include "resolve_type.hpp"
#include "storage/chunk.hpp"
#include "storage/fitted_attribute_vector.hpp"
#include "storage/storage_manager.hpp"
#include "utils/assert.hpp"

namespace opossum {

ImportBinary::ImportBinary(const std::string& filename, const std::optional<std::string> tablename)
    : _filename(filename), _tablename(tablename) {}

const std::string ImportBinary::name() const { return "ImportBinary"; }

template <typename T>
pmr_vector<T> ImportBinary::_read_values(std::ifstream& file, const size_t count) {
  pmr_vector<T> values(count);
  file.read(reinterpret_cast<char*>(values.data()), values.size() * sizeof(T));
  return values;
}

// specialized implementation for string values
template <>
pmr_vector<std::string> ImportBinary::_read_values(std::ifstream& file, const size_t count) {
  return _read_string_values(file, count);
}

// specialized implementation for bool values
template <>
pmr_vector<bool> ImportBinary::_read_values(std::ifstream& file, const size_t count) {
  pmr_vector<BoolAsByteType> readable_bools(count);
  file.read(reinterpret_cast<char*>(readable_bools.data()), readable_bools.size() * sizeof(BoolAsByteType));
  return pmr_vector<bool>(readable_bools.begin(), readable_bools.end());
}

template <typename T>
pmr_vector<std::string> ImportBinary::_read_string_values(std::ifstream& file, const size_t count) {
  const auto string_lengths = _read_values<T>(file, count);
  const auto total_length = std::accumulate(string_lengths.cbegin(), string_lengths.cend(), static_cast<size_t>(0));
  const auto buffer = _read_values<char>(file, total_length);

  pmr_vector<std::string> values(count);
  size_t start = 0;

  for (size_t i = 0; i < count; ++i) {
    values[i] = std::string(buffer.data() + start, buffer.data() + start + string_lengths[i]);
    start += string_lengths[i];
  }

  return values;
}

template <typename T>
T ImportBinary::_read_value(std::ifstream& file) {
  T result;
  file.read(reinterpret_cast<char*>(&result), sizeof(T));
  return result;
}

std::shared_ptr<const Table> ImportBinary::_on_execute() {
  if (_tablename && StorageManager::get().has_table(*_tablename)) {
    return StorageManager::get().get_table(*_tablename);
  }

  std::ifstream file;
  file.open(_filename, std::ios::binary);

  Assert(file.is_open(), "ImportBinary: Could not find file " + _filename);

  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  std::shared_ptr<Table> table;
  ChunkID chunk_count;
  std::tie(table, chunk_count) = _read_header(file);
  for (ChunkID chunk_id{0}; chunk_id < chunk_count; ++chunk_id) {
    table->emplace_chunk(_import_chunk(file, table));
  }

  if (_tablename) {
    StorageManager::get().add_table(*_tablename, table);
  }

  return table;
}

std::pair<std::shared_ptr<Table>, ChunkID> ImportBinary::_read_header(std::ifstream& file) {
  const auto chunk_size = _read_value<ChunkOffset>(file);
  const auto chunk_count = _read_value<ChunkID>(file);
  const auto column_count = _read_value<ColumnID>(file);
  const auto data_types = _read_values<std::string>(file, column_count);
  const auto column_nullables = _read_values<bool>(file, column_count);
  const auto column_names = _read_string_values<ColumnNameLength>(file, column_count);

  auto table = std::make_shared<Table>(chunk_size);

  // Add columns to table
  for (ColumnID column_id{0}; column_id < column_count; ++column_id) {
    const auto data_type = data_type_to_string.right.at(data_types[column_id]);

    table->add_column_definition(column_names[column_id], data_type, column_nullables[column_id]);
  }

  return std::make_pair(table, chunk_count);
}

std::shared_ptr<Chunk> ImportBinary::_import_chunk(std::ifstream& file, std::shared_ptr<Table>& table) {
  const auto row_count = _read_value<ChunkOffset>(file);
  const auto chunk = std::make_shared<Chunk>(ChunkUseMvcc::Yes);

  for (ColumnID column_id{0}; column_id < table->column_count(); ++column_id) {
    chunk->add_column(
        _import_column(file, row_count, table->column_type(column_id), table->column_is_nullable(column_id)));
  }
  return chunk;
}

std::shared_ptr<BaseColumn> ImportBinary::_import_column(std::ifstream& file, ChunkOffset row_count, DataType data_type,
                                                         bool is_nullable) {
  std::shared_ptr<BaseColumn> result;
  resolve_data_type(data_type, [&](auto type) {
    using ColumnDataType = typename decltype(type)::type;
    result = _import_column<ColumnDataType>(file, row_count, is_nullable);
  });

  return result;
}

template <typename ColumnDataType>
std::shared_ptr<BaseColumn> ImportBinary::_import_column(std::ifstream& file, ChunkOffset row_count, bool is_nullable) {
  const auto column_type = _read_value<BinaryColumnType>(file);

  switch (column_type) {
    case BinaryColumnType::value_column:
      return _import_value_column<ColumnDataType>(file, row_count, is_nullable);
    case BinaryColumnType::dictionary_column:
      return _import_dictionary_column<ColumnDataType>(file, row_count);
    default:
      // This case happens if the read column type is not a valid BinaryColumnType.
      Fail("Cannot import column: invalid column type");
  }
}

std::shared_ptr<BaseAttributeVector> ImportBinary::_import_attribute_vector(
    std::ifstream& file, ChunkOffset row_count, AttributeVectorWidth attribute_vector_width) {
  switch (attribute_vector_width) {
    case 1:
      return std::make_shared<FittedAttributeVector<uint8_t>>(_read_values<uint8_t>(file, row_count));
    case 2:
      return std::make_shared<FittedAttributeVector<uint16_t>>(_read_values<uint16_t>(file, row_count));
    case 4:
      return std::make_shared<FittedAttributeVector<uint32_t>>(_read_values<uint32_t>(file, row_count));
    default:
      Fail("Cannot import attribute vector with width: " + std::to_string(attribute_vector_width));
  }
}

template <typename T>
std::shared_ptr<ValueColumn<T>> ImportBinary::_import_value_column(std::ifstream& file, ChunkOffset row_count,
                                                                   bool is_nullable) {
  // TODO(unknown): Ideally _read_values would directly write into a tbb::concurrent_vector so that no conversion is
  // needed
  if (is_nullable) {
    const auto nullables = _read_values<bool>(file, row_count);
    const auto values = _read_values<T>(file, row_count);
    return std::make_shared<ValueColumn<T>>(tbb::concurrent_vector<T>{values.begin(), values.end()},
                                            tbb::concurrent_vector<bool>{nullables.begin(), nullables.end()});
  } else {
    const auto values = _read_values<T>(file, row_count);
    return std::make_shared<ValueColumn<T>>(tbb::concurrent_vector<T>{values.begin(), values.end()});
  }
}

template <typename T>
std::shared_ptr<DictionaryColumn<T>> ImportBinary::_import_dictionary_column(std::ifstream& file,
                                                                             ChunkOffset row_count) {
  const auto attribute_vector_width = _read_value<AttributeVectorWidth>(file);
  const auto dictionary_size = _read_value<ValueID>(file);
  auto dictionary = _read_values<T>(file, dictionary_size);
  auto attribute_vector = _import_attribute_vector(file, row_count, attribute_vector_width);
  return std::make_shared<DictionaryColumn<T>>(std::move(dictionary), std::move(attribute_vector));
}

}  // namespace opossum
