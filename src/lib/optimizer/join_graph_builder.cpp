#include "join_graph_builder.hpp"

#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "utils/assert.hpp"

namespace opossum {

JoinGraph JoinGraphBuilder::build_join_graph(const std::shared_ptr<AbstractLQPNode>& root) {
  _traverse(root, true);

  /**
   * Generate JoinEdges
   */

  for (const auto& predicate : _predicates) {
    if (predicate.is_column_predicate()) {
      auto vertex_left = _get_vertex_for_column_origin(predicate.column_origin);
      auto vertex_right = _get_vertex_for_column_origin(boost::get<LQPColumnOrigin>(predicate.value));

      if (vertex_left == vertex_right) {
        vertex_left->predicates.emplace_back(predicate);
      } else {
        const auto join_predicate = JoinPredicate{JoinMode::Inner, JoinColumnOrigins{predicate.column_origin, boost::get<LQPColumnOrigin>(predicate.value)}, predicate.scan_type};

        auto edge = _get_or_create_edge({vertex_left, vertex_right});
        edge->predicates.emplace_back(join_predicate);
      }
    } else if (predicate.is_value_predicate()) {
      auto vertex = _get_vertex_for_column_origin(predicate.column_origin);
      vertex->predicates.emplace_back(predicate);
    }
  }

  // Just make sure for every CrossJoin there is a JoinEdge
  for (const auto& cross_join : _cross_joins) {
    _get_or_create_edge({cross_join.first, cross_join.second});
  }

  return {std::move(_vertices), std::move(_edges)};
}

void JoinGraphBuilder::_traverse(const std::shared_ptr<AbstractLQPNode>& node, const bool is_root_invocation) {
  /**
   * Early return to make it possible to call search_join_graph() on both children without having to check whether they
   * are nullptr.
   */
  if (!node) {
    return;
  }

  std::vector<std::shared_ptr<JoinVertex>> vertices;

  // Except for the root invocation, all nodes with multiple parents become vertices
  if (!is_root_invocation && node->num_parents() > 1) {
    _vertices.emplace_back(std::make_shared<JoinVertex>(node));
    return;
  }

  if (node->type() == LQPNodeType::Join) {
    const auto join_node = std::static_pointer_cast<const JoinNode>(node);

    if (join_node->join_mode() == JoinMode::Inner) {
      _predicates.emplace_back(join_node->join_column_origins()->first, *join_node->scan_type(), join_node->join_column_origins()->second);
    } else if (join_node->join_mode() == JoinMode::Cross) {
      /**
       * Create a cross join from any one vertex in the left subtree with any one vertex in the right subtree
       */

      _traverse(node->left_child(), false);
      const auto left_vertex = _vertices.back();
      _traverse(node->right_child(), false);
      const auto right_vertex = _vertices.back();

      _cross_joins.emplace_back(left_vertex, right_vertex);

      return ; // We already traversed the children
    } else {
      // We're only turning Cross and Inner Joins into JoinPredicates in the JoinGraph right now
      _vertices.emplace_back(std::make_shared<JoinVertex>(node));
      return;
    }
  } else if (node->type() == LQPNodeType::Predicate) {
    const auto predicate_node = std::static_pointer_cast<const PredicateNode>(node);

    // A non-column-to-column Predicate becomes a vertex
    if (is_lqp_column_origin(predicate_node->value())) {
      _predicates.emplace_back(predicate_node->column_origin(), predicate_node->scan_type(), predicate_node->value());
    } else {
      _predicates.emplace_back(predicate_node->column_origin(), predicate_node->scan_type(), predicate_node->value(), predicate_node->value2());
    }
  } else {
    // Everything that is not a Join or a Predicate becomes a vertex
    _vertices.emplace_back(std::make_shared<JoinVertex>(node));
    return;
  }

  _traverse(node->left_child(), false);
  _traverse(node->right_child(), false);
}

std::shared_ptr<JoinEdge> JoinGraphBuilder::_get_or_create_edge(const JoinVertexPair& vertices) {
  auto ordered_vertices = vertices;
  if (ordered_vertices.first > ordered_vertices.second) {
    std::swap(ordered_vertices.first, ordered_vertices.second);
  }

  auto iter = _edge_idx_by_vertices.find(ordered_vertices);
  if (iter == _edge_idx_by_vertices.end()) {
    iter = _edge_idx_by_vertices.emplace(ordered_vertices, _edges.size()).first;
    _edges.emplace_back(std::make_shared<JoinEdge>(ordered_vertices));
  }

  return _edges[iter->second];
}

std::shared_ptr<JoinVertex> JoinGraphBuilder::_get_vertex_for_column_origin(const LQPColumnOrigin &column_origin) const {
  auto iter = _vertex_of_node.find(column_origin.node());

  if (iter == _vertex_of_node.end()) {
    auto iter2 = std::find_if(_vertices.begin(), _vertices.end(), [&](const auto& vertex) {
      return vertex->node->find_output_column_id_by_column_origin(column_origin).has_value();
    });
    Assert(iter2 != _vertices.end(), "Should have found ColumnOrigin. Bug!");
    iter = _vertex_of_node.emplace(column_origin.node(), *iter2).first;
  }

  return iter->second;
}

}