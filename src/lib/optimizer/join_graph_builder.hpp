#pragma once

#include <memory>
#include <vector>

#include "join_graph.hpp"

namespace opossum {

class AbstractLQPNode;

/**
 * For building JoinGraphs from ASTs
 */
class JoinGraphBuilder final {
 public:
  /**
   * From the subtree of root, build a JoinGraph.
   * The LQP is not modified during this process.
   */
  std::shared_ptr<JoinGraph> build_join_graph(const std::shared_ptr<const AbstractLQPNode>& node);

//  /**
//   * Recursively search node for JoinGraphs
//   */
//  static std::vector<std::shared_ptr<JoinGraph>> build_all_join_graphs(const std::shared_ptr<AbstractLQPNode>& node);

 private:
//  /**
//   * Implementation backend of the public function build_all_join_graphs().
//   */
//  static std::vector<std::shared_ptr<JoinGraph>> _build_all_join_graphs(const std::shared_ptr<AbstractASTNode>& node,
//                                                                        std::unordered_set<std::shared_ptr<AbstractASTNode>> & visited_nodes);

  /**
   * Helper method/actual implementation for build_join_graph().
   * @param node                The subtree to investigate
   * @param o_vertices          Output parameter, collecting all the vertex-AST-nodes
   * @param o_edges             Output parameter, collecting all edges/predicates in the tree
   * @param is_root_invocation  In the root multiple parents are allowed, everywhere else they are not and such nodes
   *                            will be considered vertices
   */
  void _traverse_lqp_for_join_graph(const std::shared_ptr<const AbstractLQPNode>& node, const bool is_root_invocation);

  // @{
  /**
   * When building a JoinGraph, these handle (i.e build vertices and edges for) specific node types
   */
  void _traverse_inner_join_node(const std::shared_ptr<const JoinNode>& node);
  void _traverse_cross_join_node(const std::shared_ptr<const JoinNode>& node);
  void _traverse_column_predicate_node(const std::shared_ptr<const PredicateNode>& node);
  void _traverse_value_predicate_node(const std::shared_ptr<const PredicateNode>& node);
  // @}

  /**
   * Within the index range [vertex_range_begin, vertex_range_end) in vertices, look for the `column_id`th column and
   * return the index of the Vertex it belongs to, as well as the ColumnID in that vertex
   */
  static std::pair<JoinVertexID, ColumnID> _find_vertex_and_column_id(const JoinGraph::Vertices& vertices, ColumnID column_id,
                                                                      JoinVertexID vertex_range_begin,
                                                                      JoinVertexID vertex_range_end);

  JoinGraph::Vertices _vertices;
  JoinGraph::JoinPredicates _join_predicates;
};

}