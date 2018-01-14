#include <iostream>

#include "storage/storage_manager.hpp"
#include "logical_query_plan/stored_table_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "optimizer/strategy/join_ordering/dplinear.hpp"
#include "optimizer/join_graph_builder.hpp"
#include "utils/load_table.hpp"

using namespace opossum;  // NOLINT

int main() {
  StorageManager::get().add_table("int_int_int_100", load_table("src/test/tables/sqlite/int_int_int_100.tbl", 20));

  auto stored_table_node_a = std::make_shared<StoredTableNode>("int_int_int_100");
  auto stored_table_node_b = std::make_shared<StoredTableNode>("int_int_int_100");
  auto stored_table_node_c = std::make_shared<StoredTableNode>("int_int_int_100");

  const auto a_a = LQPColumnOrigin{stored_table_node_a, ColumnID{0}};
  const auto b_b = LQPColumnOrigin{stored_table_node_b, ColumnID{1}};
  const auto c_c = LQPColumnOrigin{stored_table_node_c, ColumnID{2}};

  auto join_node_a = std::make_shared<JoinNode>(JoinMode::Inner, JoinColumnOrigins{a_a, b_b}, ScanType::GreaterThan);
  auto join_node_b = std::make_shared<JoinNode>(JoinMode::Inner, JoinColumnOrigins{a_a, c_c}, ScanType::GreaterThan);

  join_node_b->set_left_child(join_node_a);
  join_node_b->set_right_child(stored_table_node_c);
  join_node_a->set_left_child(stored_table_node_a);
  join_node_a->set_right_child(stored_table_node_b);

  join_node_b->print();

  const auto join_graph = JoinGraphBuilder{}.build_join_graph(join_node_b);
  join_graph.print();

  const auto optimized_lqp = DPLinear{}.run(std::make_shared<JoinGraph>(join_graph));

  return 0;
}
