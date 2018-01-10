#pragma once

#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/lqp_predicate.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "types.hpp"

namespace opossum {

class JoinNode;
class PredcicateNode;
struct JoinVertex;

/**
 * A connection between two JoinGraph-Vertices.
 */
struct JoinPredicate final {
  JoinPredicate(JoinMode join_mode, const JoinColumnOrigins& join_column_origins, ScanType scan_type);

  const JoinMode join_mode;
  const std::optional<const JoinColumnOrigins> join_column_origins;
  const std::optional<const ScanType> scan_type;
};

struct JoinVertex final {
  explicit JoinVertex(const std::shared_ptr<const AbstractLQPNode>& node);

  std::shared_ptr<const AbstractLQPNode> node;
  std::vector<LQPPredicate> predicates;
};

using JoinVertexPair = std::pair<std::shared_ptr<JoinVertex>, std::shared_ptr<JoinVertex>>;

struct JoinEdge final {
  JoinEdge(const JoinVertexPair& vertices);

  JoinVertexPair vertices;
  std::vector<JoinPredicate> predicates;
};

/**
 * Describes a set of AST subtrees (called "vertices") and the predicates (called "edges") they are connected with.
 * JoinGraphs are the core data structure worked on during JoinOrdering.
 * A JoinGraph is a unordered representation of a JoinPlan, i.e. a AST subtree that consists of Joins,
 * Predicates and Leafs (which are all other kind of nodes).
 *
 * See the tests for examples.
 */
struct JoinGraph final {
 public:
  using Vertices = std::vector<std::shared_ptr<JoinVertex>>;
  using Edges = std::vector<JoinEdge>;

  JoinGraph(Vertices vertices, Edges edges);

  Vertices vertices;
  Edges edges;
};
} // namespace opossum