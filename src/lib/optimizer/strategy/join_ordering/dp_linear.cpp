//#include "dp_linear.hpp"
//
//#include <set>
//
//#include "optimizer/join_graph.hpp"
//#include "optimizer/table_statistics.hpp"
//
//namespace {
//
//void _generate_all_subsets_n(std::vector<std::set<size_t>>& subsets,
//                             std::set<size_t>& current_subset,
//                             size_t index, size_t set_size, size_t subset_size) {
//  if (current_subset.size() == subset_size) {
//    subsets.emplace_back(current_subset);
//    return;
//  }
//
//  if (index == set_size) return;
//
//  auto current_subset_copy = current_subset;
//  current_subset.insert(index);
//
//  _generate_all_subsets_n(subsets, current_subset, index + 1, set_size, subset_size);
//  _generate_all_subsets_n(subsets, current_subset_copy, index + 1, set_size, subset_size);
//}
//
//std::vector<std::set<size_t>> generate_all_subsets_n(const size_t set_size, const size_t subset_size) {
//  std::vector<std::set<size_t>> subsets;
//  std::set<size_t> current_subset;
//
//  _generate_all_subsets_n(subsets, current_subset, 0, set_size, subset_size);
//
//  return subsets;
//}
//
//}
//
//namespace opossum {
//
//DPLinear::DPLinear(const std::shared_ptr<const JoinGraph>& join_graph): _join_graph(join_graph) {}
//
//std::shared_ptr<AbstractLQPNode> DPLinear::run() {
//  const auto num_vertices = _join_graph->vertices.size();
//
//  for (size_t vertex_idx = 0; vertex_idx < num_vertices; ++vertex_idx) {
//    std::set<size_t> single_vertex_idx;
//    single_vertex_idx.insert(vertex_idx);
//
//    const auto single_vertex_tree = std::make_shared<JoinTree>(_join_graph->vertices[vertex_idx]->node, std::vector<std::shared_ptr<JoinVertex>>{_join_graph->vertices[vertex_idx]});
//
//    _set_best_tree(single_vertex_idx, single_vertex_tree);
//  }
//
//  for (size_t num_vertices_in_tree = 1; num_vertices_in_tree < num_vertices; ++num_vertices_in_tree) {
//    const auto vertex_idx_subsets = generate_all_subsets_n(num_vertices, num_vertices_in_tree);
//
//    for (const auto& vertex_idx_subset : vertex_idx_subsets) {
//      for (size_t join_vertex_idx = 0; join_vertex_idx < num_vertices; ++join_vertex_idx) {
//        // JoinTree already contains this vertex
//        if (vertex_idx_subset.count(join_vertex_idx) != 0) continue;
//
//        const auto join_vertex = _join_graph->vertices[join_vertex_idx];
//        const auto subset_best_tree = _best_tree(vertex_idx_subset);
//        if (!subset_best_tree) {
//          continue; // Subtree is not connected
//        }
//
//        const auto join_tree = _create_join_tree(subset_best_tree, join_vertex);
//        if (!join_tree) {
//          continue;
//        }
//
//        auto joined_vertex_idx_subset = vertex_idx_subset;
//        joined_vertex_idx_subset.insert(join_vertex_idx);
//
//
//        const auto current_best_tree = _best_tree(joined_vertex_idx_subset);
//
//        if (current_best_tree == nullptr || _cost(current_best_tree->lqp) > _cost(join_tree->lqp)) {
//          std::cout << "Setting tree for "; for (auto i : vertex_idx_subset) std::cout << i << " "; std::cout << " - joined with " << join_vertex_idx << std::endl;
//          _set_best_tree(joined_vertex_idx_subset, join_tree);
//        }
//      }
//    }
//  }
//
//  std::set<size_t> all_vertex_ids;
//  for (size_t vertex_idx = 0; vertex_idx < num_vertices; ++vertex_idx) all_vertex_ids.insert(vertex_idx);
//
//  return _best_tree(all_vertex_ids)->lqp;
//}
//
//}  // namespace opossum
//
