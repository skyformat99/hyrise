#pragma once

#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "types.hpp"

namespace opossum {

class JoinNode;
class PredcicateNode;
struct JoinVertex;

using JoinLQPNodes = std::pair<std::shared_ptr<AbstractLQPNode>, std::shared_ptr<AbstractLQPNode>>;

/**
 * A connection between two JoinGraph-Vertices.
 */
struct LQPJoinPredicate final {
  /**
   * Construct NATURAL or self join edge
   */
  LQPJoinPredicate(const JoinLQPNodes& join_node, const JoinMode join_mode);

  /**
   * Construct a predicates JoinEdge
   */
  LQPJoinPredicate(const JoinColumnOrigins& join_column_origins, JoinMode join_mode, ScanType scan_type);

  std::string description(DescriptionMode description_mode = DescriptionMode::SingleLine) const;

  const JoinLQPNodes join_nodes;
  const JoinMode join_mode;
  const std::optional<const JoinColumnOrigins> join_column_origins;
  const std::optional<const ScanType> scan_type;
};

/**
 * Predicate on a single node. value2 will only be engaged, if scan_type is ScanType::Between.
 */
struct LQPNodePredicate final {
  LQPNodePredicate(const LQPColumnOrigin& column_origin,
                   const ScanType scan_type,
                   const AllParameterVariant& value,
                   const std::optional<AllTypeVariant>& value2);

  std::string description() const;

  const LQPColumnOrigin column_origin;
  const ScanType scan_type;
  const AllParameterVariant value;
  const std::optional<const AllTypeVariant> value2;
};

struct JoinVertex {
  explicit JoinVertex(const std::shared_ptr<AbstractLQPNode>& node);

  std::string description() const;

  std::shared_ptr<AbstractLQPNode> node;
  std::vector<LQPNodePredicate> predicates;
};

/**
 * Describes a set of AST subtrees (called "vertices") and the predicates (called "edges") they are connected with.
 * JoinGraphs are the core data structure worked on during JoinOrdering.
 * A JoinGraph is a unordered representation of a JoinPlan, i.e. a AST subtree that consists of Joins,
 * Predicates and Leafs (which are all other kind of nodes).
 *
 * See the tests for examples.
 */
class JoinGraph final {
 public:
  using Vertices = std::vector<JoinVertex>;
  using JoinPredicates = std::vector<LQPJoinPredicate>;

  JoinGraph() = default;
  JoinGraph(Vertices vertices, JoinPredicates predicates);

  const Vertices& vertices() const;
  const JoinPredicates& join_predicates() const;

  void print(std::ostream& out = std::cout) const;

 private:
  Vertices _vertices;
  JoinPredicates _predicates;
};
} // namespace opossum