#pragma once

#include <memory>
#include <set>
#include <vector>

#include "join_graph.hpp"
#include "logical_query_plan/lqp_predicate.hpp"

namespace opossum {

class AbstractLQPNode;


/**
 * For building JoinGraphs from ASTs
 */
class JoinGraphBuilder final {
 public:
  using JoinVertexPair = std::pair<std::shared_ptr<JoinVertex>, std::shared_ptr<JoinVertex>>;

  /**
   * From the subtree of root, build a JoinGraph.
   * The LQP is not modified during this process.
   */
  JoinGraph build_join_graph(const std::shared_ptr<const AbstractLQPNode>& node);

//  /**
//   * Recursively search node for JoinGraphs
//   */
//  static std::vector<std::shared_ptr<JoinGraph>> build_all_join_graphs(const std::shared_ptr<AbstractLQPNode>& node);

 private:
  void _traverse(const std::shared_ptr<const AbstractLQPNode>& node, const bool is_root_invocation);
  void _traverse_children(const std::shared_ptr<const AbstractLQPNode>& node);

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
   * @return JoinEdge, might be invalidated by subsequent call
   */
  JoinEdge& _get_or_create_edge(const JoinVertexPair& vertices);

  std::shared_ptr<JoinVertex> _find_vertex_for_column_origin(const LQPColumnOrigin& column_origin) const;

  JoinGraph::Vertices _vertices;
  std::vector<LQPPredicate> _predicates;
  std::vector<JoinVertexPair> _cross_joins;
  std::vector<JoinEdge> _edges;

  std::unordered_map<std::shared_ptr<JoinVertex>, size_t> _edge_ids_by_vertex;
};

}