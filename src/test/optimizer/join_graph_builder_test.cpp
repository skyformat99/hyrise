#include "gtest/gtest.h"

#include <memory>

#include "logical_query_plan/aggregate_node.hpp"
#include "logical_query_plan/mock_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/sort_node.hpp"
#include "logical_query_plan/union_node.hpp"
#include "logical_query_plan/validate_node.hpp"
#include "optimizer/join_graph_builder.hpp"

namespace {

using namespace opossum;  // NOLINT

bool edge_has_predicate(const std::shared_ptr<JoinEdge>& edge, const JoinColumnOrigins& join_column_origins, ScanType scan_type) {
  const auto join_predicate = JoinPredicate{JoinMode::Inner, join_column_origins, scan_type};

  return std::find(edge->predicates.begin(), edge->predicates.end(), join_predicate) != edge->predicates.end() ||
  std::find(edge->predicates.begin(), edge->predicates.end(), join_predicate.flipped()) != edge->predicates.end();
}
bool vertex_has_predicate(const std::shared_ptr<JoinVertex>& vertex, const LQPPredicate& predicate) {
  return std::find(vertex->predicates.begin(), vertex->predicates.end(), predicate) != vertex->predicates.end();
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

  const auto join_graph = JoinGraphBuilder{}.build_join_graph(lqp);

  /**
   * Check the JoinVertices
   */
  EXPECT_EQ(join_graph.vertices.size(), 4u);

  const auto vertex_a = join_graph.find_vertex(mock_node_a);
  const auto vertex_b = join_graph.find_vertex(mock_node_b);
  const auto vertex_c = join_graph.find_vertex(mock_node_c);
  const auto vertex_d = join_graph.find_vertex(mock_node_d);

  EXPECT_NE(vertex_a, nullptr);
  EXPECT_NE(vertex_b, nullptr);
  EXPECT_NE(vertex_c, nullptr);
  EXPECT_NE(vertex_d, nullptr);

  EXPECT_EQ(vertex_a->predicates.size(), 0u);
  EXPECT_EQ(vertex_b->predicates.size(), 0u);
  EXPECT_EQ(vertex_c->predicates.size(), 1u);
  EXPECT_TRUE(vertex_has_predicate(vertex_c, LQPPredicate{column_c, ScanType::Equals, 42}));
  EXPECT_EQ(vertex_d->predicates.size(), 1u);
  EXPECT_TRUE(vertex_has_predicate(vertex_d, LQPPredicate{column_d, ScanType::LessThan, 42}));

  /**
   * Check the JoinEdges
   */
  // TODO(moritz) among these JoinEdges is one CrossJoin Edge between either a-c, a-d, b-c or b-d. Test for it.
  EXPECT_EQ(join_graph.edges.size(), 4u);

  const auto edge_a_b = join_graph.find_edge({mock_node_a, mock_node_b});
  ASSERT_NE(edge_a_b, nullptr);
  EXPECT_EQ(edge_a_b->predicates.size(), 2u);
  EXPECT_TRUE(edge_has_predicate(edge_a_b, JoinColumnOrigins{column_a, column_b1}, ScanType::Equals));
  EXPECT_TRUE(edge_has_predicate(edge_a_b, JoinColumnOrigins{column_a, column_b2}, ScanType::GreaterThan));

  const auto edge_c_d = join_graph.find_edge({mock_node_c, mock_node_d});
  ASSERT_NE(edge_c_d, nullptr);
  EXPECT_EQ(edge_c_d->predicates.size(), 1u);
  EXPECT_TRUE(edge_has_predicate(edge_c_d, JoinColumnOrigins{column_c, column_d}, ScanType::Equals));

  const auto edge_a_d = join_graph.find_edge({mock_node_a, mock_node_d});
  ASSERT_NE(edge_a_d, nullptr);
  EXPECT_EQ(edge_a_d->predicates.size(), 1u);
  EXPECT_TRUE(edge_has_predicate(edge_a_d, JoinColumnOrigins{column_a, column_d}, ScanType::Equals));
}

TEST_F(JoinGraphBuilderTest, NodeTypesBecomingVertices) {
  /**
   * Test that everything except Joins and Predicates becomes a vertex for now
   */

  /*
     [0] [UnionNode] Mode: UnionPositions
     \_[1] [Predicate] B.y = 42
     |  \_[2] [Cross Join]
     |     \_[3] [Aggregate] SUM(A.x1) GROUP BY [A.x2]
     |     \_[4] [Cross Join]
     |        \_[5] [Sort] B.y (Ascending)
     |        |  \_[6] [MockTable 'B'] -- ALIAS: 'B'
     |        \_[7] [Validate]
     |           \_[8] [MockTable 'C'] -- ALIAS: 'C'
     \_[9] [Predicate] C.z = 42
        \_Recurring Node --> [2]
   */

  const auto mock_node_a = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "x1"}, {DataType::Int, "x2"}}, "A");
  const auto mock_node_b = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "y"}}, "B");
  const auto mock_node_c = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "z"}}, "C");

  const auto x1 = LQPColumnOrigin{mock_node_a, ColumnID{0}};
  const auto x2 = LQPColumnOrigin{mock_node_a, ColumnID{1}};
  const auto y = LQPColumnOrigin{mock_node_b, ColumnID{0}};
  const auto z = LQPColumnOrigin{mock_node_c, ColumnID{0}};

  std::vector<std::shared_ptr<LQPExpression>> aggregates{LQPExpression::create_aggregate_function(AggregateFunction::Sum, {LQPExpression::create_column(x1)})};
  const auto aggregate_node = std::make_shared<AggregateNode>(aggregates, std::vector<LQPColumnOrigin>{x2});

  const auto sort_node = std::make_shared<SortNode>(OrderByDefinitions{{y, OrderByMode::Ascending}});
  const auto validate_node = std::make_shared<ValidateNode>();

  const auto join_node_a = std::make_shared<JoinNode>(JoinMode::Cross);
  const auto join_node_b = std::make_shared<JoinNode>(JoinMode::Cross);

  const auto predicate_node_a = std::make_shared<PredicateNode>(y, ScanType::Equals, 42);
  const auto predicate_node_b = std::make_shared<PredicateNode>(z, ScanType::Equals, 42);

  const auto union_node = std::make_shared<UnionNode>(UnionMode::Positions);

  union_node->set_left_child(predicate_node_a);
  union_node->set_right_child(predicate_node_b);
  predicate_node_a->set_left_child(join_node_a);
  predicate_node_b->set_left_child(join_node_a);
  join_node_a->set_left_child(aggregate_node);
  join_node_a->set_right_child(join_node_b);
  aggregate_node->set_left_child(mock_node_a);
  join_node_b->set_left_child(sort_node);
  join_node_b->set_right_child(validate_node);
  sort_node->set_left_child(mock_node_b);
  validate_node->set_left_child(mock_node_c);

  /**
   * Verify the correct Vertices are being generated when building the JoinGraph from different nodes.
   */

  const auto join_graph_a = JoinGraphBuilder{}.build_join_graph(union_node);
  ASSERT_EQ(join_graph_a.vertices.size(), 1u);
  EXPECT_EQ(join_graph_a.vertices.at(0)->node, union_node);
  EXPECT_EQ(join_graph_a.edges.size(), 0u);

  const auto join_graph_b = JoinGraphBuilder{}.build_join_graph(predicate_node_a);
  ASSERT_EQ(join_graph_b.vertices.size(), 1u);
  EXPECT_EQ(join_graph_b.vertices.at(0)->node, join_node_a);
  EXPECT_EQ(join_graph_b.edges.size(), 0u);

  const auto join_graph_c = JoinGraphBuilder{}.build_join_graph(predicate_node_b);
  ASSERT_EQ(join_graph_c.vertices.size(), 1u);
  EXPECT_EQ(join_graph_c.vertices.at(0)->node, join_node_a);
  EXPECT_EQ(join_graph_c.edges.size(), 0u);

  const auto join_graph_d = JoinGraphBuilder{}.build_join_graph(join_node_a);
  ASSERT_EQ(join_graph_d.vertices.size(), 3u);
  EXPECT_NE(join_graph_d.find_vertex(aggregate_node), nullptr);
  EXPECT_NE(join_graph_d.find_vertex(sort_node), nullptr);
  EXPECT_NE(join_graph_d.find_vertex(validate_node), nullptr);
}

}  // namespace opossum