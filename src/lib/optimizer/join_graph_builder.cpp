#include "join_graph_builder.hpp"

#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "utils/assert.hpp"

namespace opossum {

JoinGraph JoinGraphBuilder::build_join_graph(const std::shared_ptr<const AbstractLQPNode>& root) {
  _traverse(root, true);

  /**
   * Generate JoinEdges
   */

  for (const auto& predicate : _column_predicates) {
    const auto vertex_left = _find_vertex_for_column_origin(predicate.join_column_origins->first);
    const auto vertex_right = _find_vertex_for_column_origin(predicate.join_column_origins->second);

    if (vertex_left == vertex_right) {
      vertex_left->predicates.emplace_back(predicate);
    } else {
      auto& edge = _get_or_create_edge(vertex_left, vertex_right);
      edge.predicates.emplace_back(predicate);
    }
  }

  for (const auto& cross_join : _cross_joins) {
    get_or_create_edge(cross_join.first, cross_join.second);
  }

  return {std::move(_vertices), std::move(_edges)};
}

void JoinGraphBuilder::_traverse(const std::shared_ptr<const AbstractLQPNode>& node, const bool is_root_invocation) {
  /**
   * Early return to make it possible to call search_join_graph() on both children without having to check whether they
   * are nullptr.
   */
  if (!node) {
    return {};
  }

  std::vector<std::shared_ptr<JoinVertex>> vertices;

  // Except for the root invocation, all nodes with multiple parents become vertices
  if (!is_root_invocation && node->num_parents() > 1) {
    _vertices.emplace_back(node);
    return;
  }

  if (node->type() == LQPNodeType::Join) {
    const auto join_node = std::static_pointer_cast<JoinNode>(node);

    if (join_node->join_mode() == JoinMode::Inner) {
      _traverse_inner_join_node(join_node);
    } else if (join_node->join_mode() == JoinMode::Cross) {
      _traverse_cross_join_node(join_node);
    } else {
      // We're only turning Cross and Inner Joins into JoinPredicates in the JoinGraph right now
      _vertices.emplace_back(node);
      return;
    }
  } else if (node->type() == LQPNodeType::Predicate) {
    const auto predicate_node = std::static_pointer_cast<PredicateNode>(node);

    // A non-column-to-column Predicate becomes a vertex
    if (is_lqp_column_origin(predicate_node->value())) {
      _traverse_value_predicate_node(predicate_node);
    } else {
      _traverse_column_predicate_node(predicate_node);
    }
  } else {
    // Everything that is not a Join or a Predicate becomes a vertex
    _vertices.emplace_back(node);
    return;
  }
}

void JoinGraphBuilder::_traverse_children(const std::shared_ptr<const AbstractLQPNode>& node) {
  _traverse(node->left_child(), false);
  _traverse(node->right_child(), false);
}

void JoinGraphBuilder::_traverse_inner_join_node(const std::shared_ptr<const JoinNode>& node) {
  _column_predicates.emplace_back(JoinMode::Inner, node->join_column_origins(), node->scan_type());
}

void JoinGraphBuilder::_traverse_cross_join_node(const std::shared_ptr<const JoinNode>& node) {
  /**
   * Create a cross join from any one vertex in the left subtree with any one vertex in the right subtree
   */

  _traverse(node->left_child(), false);
  const auto left_vertex = _vertices.back();
  _traverse(node->right_child(), false);
  const auto right_vertex = _vertices.back();

  _cross_joins.emplace_back(left_vertex, right_vertex);
}

void JoinGraphBuilder::_traverse_column_predicate_node(const std::shared_ptr<const PredicateNode>& node) {
  /**
   * Turn a predicate on two columns into a JoinPredicate. Detection whether this might be a Predicate on a single
   * vertex happens later.
   */

  const auto column_origins = JoinColumnOrigins{
    node->column_origin(), boost::get<LQPColumnOrigin>(node->value())
  };

  _column_predicates.emplace_back(JoinMode::Inner, column_origins, node->scan_type());
}

void JoinGraphBuilder::_traverse_value_predicate_node(const std::shared_ptr<PredicateNode>& node) {
  _value_predicates.emplace_back(node->column_origin(), node->scan_type(), node->value(), node->value2());
}

}