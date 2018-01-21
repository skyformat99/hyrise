//#pragma once
//
//#include <bitset>
//#include <vector>
//
//#include "boost/dynamic_bitset.hpp"
//
//#include "optimizer/join_graph.hpp"
//
//namespace opossum {
//
//struct JoinTree {
//  JoinTree(const std::shared_ptr<AbstractLQPNode>& lqp, const std::vector<std::shared_ptr<JoinVertex>>& vertices): lqp(lqp), vertices(vertices) {
//
//  }
//
//  std::shared_ptr<AbstractLQPNode> lqp;
//  std::vector<std::shared_ptr<JoinVertex>> vertices;
//};
//
//using VertexIDSet = boost::dynamic_bitset<>;
//
//}  // namespace opossum