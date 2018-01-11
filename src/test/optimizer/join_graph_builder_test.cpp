#include "gtest/gtest.h"

#include <memory>

#include "logical_query_plan/mock_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "optimizer/join_graph_builder.hpp"

namespace {

using namespace opossum;  // NOLINT

bool edge_has_predicate(const std::shared_ptr<JoinEdge>& edge, const JoinColumnOrigins& join_column_origins, ScanType scan_type) {
  return std::find(edge->predicates.begin(), edge->predicates.end(), JoinPredicate{JoinMode::Inner, join_column_origins, scan_type}) != edge->predicates.end();
}
}

namespace opossum {

class JoinGraphBuilderTest: public ::testing::Test {
 protected:
};

TEST_F(JoinGraphBuilderTest, SingleValuePredicate) {
  const auto mock_node = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "a"}});
  const auto predicate_node = std::make_shared<PredicateNode>(LQPColumnOrigin{mock_node, ColumnID{0}}, ScanType::GreaterThan, 42);

  predicate_node->set_left_child(mock_node);

  const auto join_graph = JoinGraphBuilder{}.build_join_graph(predicate_node);

  join_graph.print();
}

TEST_F(JoinGraphBuilderTest, SingleColumnPredicate) {
  const auto mock_node_a = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "a"}});
  const auto mock_node_b = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "b"}});
  const auto cross_join_node = std::make_shared<JoinNode>(JoinMode::Cross);
  const auto predicate_node = std::make_shared<PredicateNode>(LQPColumnOrigin{mock_node_a, ColumnID{0}}, ScanType::GreaterThan, LQPColumnOrigin{mock_node_b, ColumnID{0}});

  predicate_node->set_left_child(cross_join_node);
  cross_join_node->set_left_child(mock_node_a);
  cross_join_node->set_right_child(mock_node_b);

  const auto join_graph = JoinGraphBuilder{}.build_join_graph(predicate_node);

  join_graph.print();
}

TEST_F(JoinGraphBuilderTest, ComplexTreeLQP) {
  /**
   * [0] [Predicate] d = a
   * \_[1] [Cross Join]
   *   \_[2] [Predicate] a > b2
   *   |  \_[3] [Inner Join] a = b1
   *   |     \_[4] [MockTable]
   *   |     \_[5] [MockTable]
   *   \_[6] [Predicate] d = c
   *      \_[7] [Predicate] d < 42
   *         \_[8] [Cross Join]
   *            \_[9] [Predicate] c = 42
   *            |  \_[10] [MockTable]
   *            \_[11] [MockTable]
   */

  const auto mock_node_a = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "a"}}, "A");
  const auto mock_node_b = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "b1"}, {DataType::Int, "b2"}}, "B");
  const auto mock_node_c = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "c"}}, "C");
  const auto mock_node_d = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "d"}}, "D");

  const auto column_a = LQPColumnOrigin{mock_node_a, ColumnID{0}};
  const auto column_b1 = LQPColumnOrigin{mock_node_b, ColumnID{0}};
  const auto column_b2 = LQPColumnOrigin{mock_node_b, ColumnID{1}};
  const auto column_c = LQPColumnOrigin{mock_node_c, ColumnID{0}};
  const auto column_d = LQPColumnOrigin{mock_node_d, ColumnID{0}};

  const auto inner_join_node = std::make_shared<JoinNode>(JoinMode::Inner, JoinColumnOrigins{column_a, column_b1}, ScanType::Equals);
  const auto predicate_node_a = std::make_shared<PredicateNode>(column_a, ScanType::GreaterThan, column_b2);
  const auto predicate_node_b = std::make_shared<PredicateNode>(column_c, ScanType::Equals, 42);
  const auto predicate_node_c = std::make_shared<PredicateNode>(column_d, ScanType::LessThan, 42);
  const auto predicate_node_d = std::make_shared<PredicateNode>(column_d, ScanType::Equals, column_c);
  const auto predicate_node_e = std::make_shared<PredicateNode>(column_d, ScanType::Equals, column_a);
  const auto cross_join_a = std::make_shared<JoinNode>(JoinMode::Cross);
  const auto cross_join_b = std::make_shared<JoinNode>(JoinMode::Cross);

  inner_join_node->set_left_child(mock_node_a);
  inner_join_node->set_right_child(mock_node_b);
  predicate_node_a->set_left_child(inner_join_node);
  cross_join_a->set_left_child(predicate_node_a);
  cross_join_a->set_right_child(predicate_node_d);
  predicate_node_e->set_left_child(cross_join_a);
  predicate_node_d->set_left_child(predicate_node_c);
  predicate_node_c->set_left_child(cross_join_b);
  cross_join_b->set_left_child(predicate_node_b);
  cross_join_b->set_right_child(mock_node_d);
  predicate_node_b->set_right_child(mock_node_c);

  const auto lqp = predicate_node_e;
  lqp->print();

  const auto join_graph = JoinGraphBuilder{}.build_join_graph(lqp);
  join_graph.print();

  const auto edge_a_b = join_graph.find_edge({mock_node_a, mock_node_b});
  ASSERT_NE(edge_a_b, nullptr);
  EXPECT_EQ(edge_a_b->predicates.size(), 2u);
  EXPECT_TRUE(edge_has_predicate(edge_a_b, JoinColumnOrigins{column_a, column_b1}, ScanType::Equals));
  EXPECT_TRUE(edge_has_predicate(edge_a_b, JoinColumnOrigins{column_a, column_b2}, ScanType::GreaterThan));

  const auto edge_c_d = join_graph.find_edge({mock_node_c, mock_node_d});
  ASSERT_NE(edge_c_d, nullptr);
  EXPECT_EQ(edge_c_d->predicates.size(), 1u);
  EXPECT_TRUE(edge_has_predicate(edge_c_d, JoinColumnOrigins{column_a, column_b1}, ScanType::Equals));


}

}  // namespace opossum