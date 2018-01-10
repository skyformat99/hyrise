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

 private:
  void _traverse(const std::shared_ptr<const AbstractLQPNode>& node, const bool is_root_invocation);

  /**
   * @return JoinEdge, might be invalidated by subsequent call
   */
  JoinEdge& _get_or_create_edge(const JoinVertexPair& vertices);

  std::shared_ptr<JoinVertex> _get_vertex_for_column_origin(const LQPColumnOrigin &column_origin) const;

  JoinGraph::Vertices _vertices;
  std::vector<LQPPredicate> _predicates;
  std::vector<JoinVertexPair> _cross_joins;
  std::vector<JoinEdge> _edges;

  // JoinVertexPair key must be ordered
  std::map<JoinVertexPair, size_t> _edge_idx_by_vertices;
  mutable std::unordered_map<std::shared_ptr<const AbstractLQPNode>, std::shared_ptr<JoinVertex>> _vertex_of_node;
};
}