//#pragma once
//
//#include <bitset>
//#include <memory>
//#include <unordered_map>
//#include <set>
//
//#include "boost/dynamic_bitset.hpp"
//
//#include "optimizer/join_graph.hpp"
//#include "join_ordering.hpp"
//
//namespace opossum {
//
//class AbstractLQPNode;
//class JoinTree;
//
//class AbstractDPAlgorithm {
// public:
//  virtual ~AbstractDPAlgorithm() = default;
//
// protected:
//  std::shared_ptr<const JoinTree> _create_join_tree(const std::shared_ptr<const JoinTree>& tree, const std::shared_ptr<JoinVertex>& join_vertex) const;
//  std::shared_ptr<const JoinTree> _best_tree(const std::set<size_t>& vertex_ids) const;
//  void _set_best_tree(const std::set<size_t>& vertex_ids, const std::shared_ptr<const JoinTree>& tree);
//  float _cost(const std::shared_ptr<AbstractLQPNode>& lqp) const;
//
//  std::shared_ptr<const JoinGraph> _join_graph;
//  std::unordered_map<VertexIDSet, std::shared_ptr<const JoinTree>> _best_trees;
//};
//
//}  // namespace opossum
