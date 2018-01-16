#include <iostream>

#include "storage/storage_manager.hpp"
#include "logical_query_plan/stored_table_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/mock_node.hpp"
#include "optimizer/strategy/join_ordering/dplinear.hpp"
#include "optimizer/column_statistics.hpp"
#include "optimizer/join_graph_builder.hpp"
#include "optimizer/table_statistics.hpp"
#include "utils/load_table.hpp"

using namespace opossum;  // NOLINT

static std::shared_ptr<TableStatistics> _make_mock_table_statistics(int32_t min, int32_t max, float row_count) {
  Assert(min <= max, "min value should be smaller than max value");

  const auto column_statistics = std::vector<std::shared_ptr<BaseColumnStatistics>>(
  {std::make_shared<ColumnStatistics<int32_t>>(ColumnID{0}, row_count, min, max, 1.0f)});

  return std::make_shared<TableStatistics>(row_count, column_statistics);
}

static std::shared_ptr<MockNode> _make_mock_node(const std::string& name, int32_t min, int32_t max, float row_count) {
  return std::make_shared<MockNode>(_make_mock_table_statistics(min, max, row_count), name);
}

int main() {
  StorageManager::get().add_table("int_int_int_100", load_table("src/test/tables/sqlite/int_int_int_100.tbl", 20));

  auto table_node_a = _make_mock_node("a", 10, 80, 300);
  auto table_node_b = _make_mock_node("b", 10, 60, 60);
  auto table_node_c = _make_mock_node("c", 50, 100, 15);
  auto table_node_d = _make_mock_node("d", 53, 57, 10);
  auto table_node_e = _make_mock_node("e", 40, 90, 600);

  auto vertex_a = std::make_shared<JoinVertex>(table_node_a);
  auto vertex_b = std::make_shared<JoinVertex>(table_node_b);
  auto vertex_c = std::make_shared<JoinVertex>(table_node_c);
  auto vertex_d = std::make_shared<JoinVertex>(table_node_d);
  auto vertex_e = std::make_shared<JoinVertex>(table_node_e);

  const auto a = LQPColumnOrigin{table_node_a, ColumnID{0}};
  const auto b = LQPColumnOrigin{table_node_b, ColumnID{0}};
  const auto c = LQPColumnOrigin{table_node_c, ColumnID{0}};
  const auto d = LQPColumnOrigin{table_node_d, ColumnID{0}};
  const auto e = LQPColumnOrigin{table_node_e, ColumnID{0}};


  vertex_a->predicates.emplace_back(a, ScanType::GreaterThan, 52);


  auto edge_a_c = std::make_shared<JoinEdge>(JoinVertexPair{vertex_a, vertex_c});
//  edge_a_c->predicates.emplace_back(JoinMode::Inner, JoinColumnOrigins{a, c}, ScanType::Equals);
  auto edge_b_c = std::make_shared<JoinEdge>(JoinVertexPair{vertex_b, vertex_c});


  auto vertices = std::vector<std::shared_ptr<JoinVertex>>{vertex_a, vertex_b, vertex_c};
  auto edges = std::vector<std::shared_ptr<JoinEdge>>{edge_a_c, edge_b_c};
  auto join_graph = std::make_shared<JoinGraph>(vertices, edges);

  DPLinear{join_graph}.run()->print();

  return 0;
}
