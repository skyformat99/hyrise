#include "join_graph_builder.hpp"

#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "utils/assert.hpp"

namespace opossum {

std::shared_ptr<JoinGraph> JoinGraphBuilder::build_join_graph(const std::shared_ptr<const AbstractLQPNode>& root) {
  _traverse_lqp_for_join_graph(root, true);
  return std::make_shared<JoinGraph>(std::move(_vertices), std::move(_join_predicates));
}

//std::vector<std::shared_ptr<JoinGraph>> JoinGraphBuilder::build_all_join_graphs(const std::shared_ptr<AbstractLQPNode>& node) {
//  std::unordered_set<std::shared_ptr<AbstractLQPNode>> visited_nodes;
//  return _build_all_join_graphs(node, visited_nodes);
//}
//
//std::vector<std::shared_ptr<JoinGraph>> JoinGraphBuilder::_build_all_join_graphs(const std::shared_ptr<AbstractLQPNode>& node,
//                                                                                 std::unordered_set<std::shared_ptr<AbstractLQPNode>> & visited_nodes) {
//  const auto already_visited = !visited_nodes.emplace(node).second;
//  if (already_visited) {
//    return {};
//  }
//
//  std::vector<std::shared_ptr<JoinGraph>> join_graphs;
//
//  auto join_graph = build_join_graph(node);
//
//  if (join_graph->vertices().size() > 1) {
//    join_graphs.emplace_back(join_graph);
//
//    for (const auto& vertex : join_graph->vertices()) {
//      auto vertex_join_graphs = _build_all_join_graphs(vertex.node, visited_nodes);
//      std::copy(vertex_join_graphs.begin(), vertex_join_graphs.end(), std::back_inserter(join_graphs));
//    }
//  } else {
//    if (node->left_child()) {
//      auto child_join_graphs = _build_all_join_graphs(node->left_child(), visited_nodes);
//      std::copy(child_join_graphs.begin(), child_join_graphs.end(), std::back_inserter(join_graphs));
//    }
//    if (node->right_child()) {
//      auto child_join_graphs = _build_all_join_graphs(node->right_child(), visited_nodes);
//      std::copy(child_join_graphs.begin(), child_join_graphs.end(), std::back_inserter(join_graphs));
//    }
//  }
//
//  return join_graphs;
//}

void JoinGraphBuilder::_traverse_lqp_for_join_graph(const std::shared_ptr<const AbstractLQPNode>& node, const bool is_root_invocation) {
  /**
   * Early return to make it possible to call search_join_graph() on both children without having to check whether they
   * are nullptr.
   */
  if (!node) {
    return;
  }

  // Except for the root invocation, all nodes with multiple parents become vertices
  if (!is_root_invocation && node->num_parents() > 1) {
    _vertices.emplace_back(node);
    return;
  }

  if (node->type() == LQPNodeType::Join) {
    const auto join_node = std::static_pointer_cast<JoinNode>(node);

    if (join_node->join_mode() == JoinMode::Inner) {
      _join_predicates.emplace_back(join_node->join_column_origins(), JoinMode::Inner, join_node->scan_type());
    } else if (join_node->join_mode() == JoinMode::Cross) {
      _join_predicates.emplace_back(std::make_pair(join_node->left_child(), join_node->right_child()));
    } else {
      // We're only turning Cross and Inner Joins into JoinPredicates in the JoinGraph right now
      _vertices.emplace_back(node);
    }
  } else if (node->type() == LQPNodeType::Predicate) {
    const auto predicate_node = std::static_pointer_cast<PredicateNode>(node);

    // A non-column-to-column Predicate becomes a vertex
    if (predicate_node->value().type() != typeid(ColumnID)) {
      _traverse_value_predicate_node(predicate_node);
    } else {
      _traverse_column_predicate_node(predicate_node);
    }
  } else {
    // Everything that is not a Join or a Predicate becomes a vertex
    _vertices.emplace_back(JoinVertex(node));
  }
}

void JoinGraphBuilder::_traverse_cross_join_node(const std::shared_ptr<JoinNode>& node, JoinGraph::Vertices& _vertices,
                                                 JoinGraph::Edges& o_edges) {
  // The first vertex in _vertices that belongs to the left subtree.
  const auto first_left_subtree_vertex_idx = make_join_vertex_id(_vertices.size());
  _traverse_ast_for_join_graph(node->left_child(), _vertices, o_edges, false);

  // The first vertex in _vertices that belongs to the right subtree.
  const auto first_right_subtree_vertex_idx = make_join_vertex_id(_vertices.size());
  _traverse_ast_for_join_graph(node->right_child(), _vertices, o_edges, false);

  /**
   * Create a unconditioned edge from one vertex in the left subtree to one vertex in the right subtree
   */
  o_edges.emplace_back(std::make_pair(first_left_subtree_vertex_idx, first_right_subtree_vertex_idx), JoinMode::Cross);
}

void JoinGraphBuilder::_traverse_column_predicate_node(const std::shared_ptr<PredicateNode>& node,
                                                       JoinGraph::Vertices& _vertices, JoinGraph::Edges& o_edges) {
  const auto first_subtree_vertex_idx = make_join_vertex_id(_vertices.size());
  _traverse_ast_for_join_graph(node->left_child(), _vertices, o_edges, false);

  const auto column_id_left = node->column_id();
  const auto column_id_right = boost::get<ColumnID>(node->value());

  auto vertex_and_column_left = _find_vertex_and_column_id(_vertices, column_id_left, first_subtree_vertex_idx,
                                                           make_join_vertex_id(_vertices.size()));
  auto vertex_and_column_right = _find_vertex_and_column_id(_vertices, column_id_right, first_subtree_vertex_idx,
                                                            make_join_vertex_id(_vertices.size()));

  o_edges.emplace_back(std::make_pair(vertex_and_column_left.first, vertex_and_column_right.first),
                       std::make_pair(vertex_and_column_left.second, vertex_and_column_right.second), JoinMode::Inner,
                       node->scan_type());
}

void JoinGraphBuilder::_traverse_value_predicate_node(const std::shared_ptr<PredicateNode>& node,
                                                      JoinGraph::Vertices& _vertices, JoinGraph::Edges& o_edges) {
  const auto first_subtree_vertex_idx = make_join_vertex_id(_vertices.size());
  _traverse_ast_for_join_graph(node->left_child(), _vertices, o_edges, false);

  auto vertex_and_column = _find_vertex_and_column_id(_vertices, node->column_id(), first_subtree_vertex_idx,
                                                      make_join_vertex_id(_vertices.size()));

  auto& vertex = _vertices[vertex_and_column.first];
  vertex.predicates.emplace_back(vertex_and_column.second, node->scan_type(), node->value());
}

std::pair<JoinVertexID, ColumnID> JoinGraphBuilder::_find_vertex_and_column_id(const JoinGraph::Vertices& vertices, ColumnID column_id,
                                                                               JoinVertexID vertex_range_begin,
                                                                               JoinVertexID vertex_range_end) {
  /**
   * This is where the magic happens.
   *
   * For example: we found an AST node that we want to turn into a JoinEdge. The AST node is referring to two
   * ColumnIDs, one in the left subtree and one in the right subtree. We need to figure out which vertices it is
   * actually referring to, in order to form an edge.
   *
   * Think of the following table being generated by the left subtree:
   *
   * 0   | 1   | 2   | 3   | 4   | 5
   * a.a | a.b | a.c | b.a | c.a | c.b
   *
   * Now, if the join_column_ids.left is "4" it is actually referring to vertex "c" (with JoinVertexID "2") and
   * ColumnID "0".
   */

  for (auto vertex_idx = vertex_range_begin; vertex_idx < vertex_range_end; ++vertex_idx) {
    const auto& vertex = vertices[vertex_idx];
    if (column_id < vertex.node->output_column_count()) {
      return std::make_pair(vertex_idx, column_id);
    }
    column_id -= vertex.node->output_column_count();
  }
  Fail("Couldn't find column_id in vertex range.");
  return std::make_pair(INVALID_JOIN_VERTEX_ID, INVALID_COLUMN_ID);
}

}