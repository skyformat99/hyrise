#pragma once

#include <bitset>
#include <memory>
#include <unordered_map>
#include <set>

#include "boost/dynamic_bitset.hpp"

#include "optimizer/join_graph.hpp"
#include "join_ordering.hpp"

namespace opossum {

class AbstractLQPNode;
class JoinPlanNode;

class AbstractDpAlgorithm {
 public:
  explicit AbstractDpAlgorithm(const std::shared_ptr<const JoinGraph>& join_graph);
  virtual ~AbstractDpAlgorithm() = default;

  virtual std::shared_ptr<AbstractLQPNode> operator()() = 0;

 protected:
  std::shared_ptr<JoinPlanNode> _get_best_plan(const boost::dynamic_bitset<>& vertices) const;
  void _set_best_plan(const boost::dynamic_bitset<>& vertices, const std::shared_ptr<JoinPlanNode>& plan);
  std::shared_ptr<JoinPlanNode> _create_join_plan(const std::shared_ptr<JoinPlanNode>& left_plan, const std::shared_ptr<JoinPlanNode>& right_plan) const;

//  std::shared_ptr<const JoinTree> _create_join_tree(const std::shared_ptr<const JoinTree>& tree, const std::shared_ptr<JoinVertex>& join_vertex) const;
//  std::shared_ptr<const JoinTree> _best_tree(const std::set<size_t>& vertex_ids) const;
//  void _set_best_tree(const std::set<size_t>& vertex_ids, const std::shared_ptr<const JoinTree>& tree);
//  float _cost(const std::shared_ptr<AbstractLQPNode>& lqp) const;

  std::shared_ptr<const JoinGraph> _join_graph;
  std::unordered_map<boost::dynamic_bitset<>, std::shared_ptr<JoinPlanNode>> _best_plans;
};

}  // namespace opossum
