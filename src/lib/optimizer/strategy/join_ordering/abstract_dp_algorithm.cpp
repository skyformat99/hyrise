//#include "abstract_dp_algorithm.hpp"
//
//namespace opossum {
//
//
//std::shared_ptr<const JoinTree> AbstractDPAlgorithm::_create_join_tree(const std::shared_ptr<const JoinTree>& tree, const std::shared_ptr<JoinVertex>& join_vertex) const {
//  std::vector<std::shared_ptr<JoinEdge>> edges;
//
//  for (const auto& tree_vertex : tree->vertices) {
//    auto edge = _join_graph->find_edge(std::make_pair(tree_vertex->node, join_vertex->node));
//    if (edge) {
//      edges.emplace_back(edge);
//    }
//  }
//
//  if (edges.empty()) {
//    return nullptr;
//  }
//
//  std::vector<JoinPredicate> join_predicates;
//  for (const auto& edge : edges) {
//    std::copy(edge->predicates.begin(), edge->predicates.end(), std::back_inserter(join_predicates));
//  }
//
//  std::shared_ptr<AbstractLQPNode> joined_lqp;
//  std::shared_ptr<AbstractLQPNode> join_node;
//
//  if (join_predicates.empty()) {
//    join_node = std::make_shared<JoinNode>(JoinMode::Cross);
//    joined_lqp = join_node;
//  } else {
//    auto join_predicate = join_predicates[0];
//
//    if (tree->lqp->find_output_column_id_by_column_origin(join_predicate.join_column_origins.first)) {
//      Assert(join_vertex->node->find_output_column_id_by_column_origin(join_predicate.join_column_origins.second), "Bug");
//    } else {
//      Assert(join_vertex->node->find_output_column_id_by_column_origin(join_predicate.join_column_origins.first), "Bug");
//      Assert(tree->lqp->find_output_column_id_by_column_origin(join_predicate.join_column_origins.second), "Bug");
//      join_predicate = join_predicate.flipped();
//    }
//
//    // TODO(moritz) Order edges appropriately
//    join_node = std::make_shared<JoinNode>(JoinMode::Inner, join_predicate.join_column_origins, join_predicate.scan_type);
//    joined_lqp = join_node;
//
//    for (size_t predicate_idx = 1; predicate_idx < join_predicates.size(); ++predicate_idx) {
//      const auto& predicate = join_predicates[predicate_idx];
//      auto predicate_node = std::make_shared<PredicateNode>(predicate.join_column_origins.first, predicate.scan_type, predicate.join_column_origins.second);
//      predicate_node->set_left_child(joined_lqp);
//      joined_lqp = predicate_node;
//    }
//  }
//
//  join_node->set_left_child(tree->lqp);
//
//  auto right_child = join_vertex->node;
//  for (const auto& predicate : join_vertex->predicates) {
//    auto predicate_node = std::make_shared<PredicateNode>(predicate.column_origin, predicate.scan_type, predicate.value, predicate.value2);
//    predicate_node->set_left_child(right_child);
//    right_child = predicate_node;
//  }
//
//  join_node->set_right_child(right_child);
//
//  auto vertices = tree->vertices;
//  vertices.emplace_back(join_vertex);
//
//  return std::make_shared<JoinTree>(joined_lqp, vertices);
//}
//
//std::shared_ptr<const JoinTree> AbstractDPAlgorithm::_best_tree(const std::set<size_t>& vertex_ids) const {
//  const auto iter = _best_trees.find(vertex_ids);
//  return iter == _best_trees.end() ? std::shared_ptr<const JoinTree>{} : iter->second;
//}
//
//void AbstractDPAlgorithm::_set_best_tree(const std::set<size_t>& vertex_ids, const std::shared_ptr<const JoinTree>& tree) {
//  _best_trees[vertex_ids] = tree;
//}
//
//float AbstractDPAlgorithm::_cost(const std::shared_ptr<AbstractLQPNode>& lqp) const {
//  const auto cost_left = lqp->left_child() ? _cost(lqp->left_child()) : 0.0f;
//  const auto cost_right = lqp->right_child() ? _cost(lqp->right_child()) : 0.0f;
//
//  auto cost = cost_left + cost_right;
//
//  if (lqp->type() == LQPNodeType::Predicate) {
//    cost += lqp->left_child()->get_statistics()->row_count();
//  } else if (lqp->type() == LQPNodeType::Join) {
//    cost += lqp->left_child()->get_statistics()->row_count() * lqp->right_child()->get_statistics()->row_count();
//  } else {
//    return lqp->get_statistics()->row_count();
//  }
//
//  return cost;
//}
//}  // namespace opossum
