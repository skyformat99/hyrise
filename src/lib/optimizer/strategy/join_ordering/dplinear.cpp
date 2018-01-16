#include "dplinear.hpp"

#include <set>

#include "optimizer/join_graph.hpp"
#include "optimizer/table_statistics.hpp"

namespace {

void _generate_all_subsets_n(std::vector<std::set<size_t>>& subsets,
                             std::set<size_t>& current_subset,
                             size_t index, size_t set_size, size_t subset_size) {
  if (current_subset.size() == subset_size) {
    subsets.emplace_back(current_subset);
    return;
  }

  if (index == set_size) return;

  auto current_subset_copy = current_subset;
  current_subset.insert(index);

  _generate_all_subsets_n(subsets, current_subset, index + 1, set_size, subset_size);
  _generate_all_subsets_n(subsets, current_subset_copy, index + 1, set_size, subset_size);
}

std::vector<std::set<size_t>> generate_all_subsets_n(const size_t set_size, const size_t subset_size) {
  std::vector<std::set<size_t>> subsets;
  std::set<size_t> current_subset;

  _generate_all_subsets_n(subsets, current_subset, 0, set_size, subset_size);

  return subsets;
}

}

namespace opossum {

DPLinear::DPLinear(const std::shared_ptr<const JoinGraph>& join_graph): _join_graph(join_graph) {}

std::shared_ptr<AbstractLQPNode> DPLinear::run() {
  const auto num_vertices = _join_graph->vertices.size();

  for (size_t vertex_idx = 0; vertex_idx < num_vertices; ++vertex_idx) {
    std::set<size_t> single_vertex_idx;
    single_vertex_idx.insert(vertex_idx);

    const auto single_vertex_tree = std::make_shared<JoinTree>(_join_graph->vertices[vertex_idx]->node, std::vector<std::shared_ptr<JoinVertex>>{_join_graph->vertices[vertex_idx]});

    _set_best_tree(single_vertex_idx, single_vertex_tree);
  }

  for (size_t num_vertices_in_tree = 1; num_vertices_in_tree < num_vertices; ++num_vertices_in_tree) {
    const auto vertex_idx_subsets = generate_all_subsets_n(num_vertices, num_vertices_in_tree);

    for (const auto& vertex_idx_subset : vertex_idx_subsets) {
      for (size_t join_vertex_idx = 0; join_vertex_idx < num_vertices; ++join_vertex_idx) {
        // JoinTree already contains this vertex
        if (vertex_idx_subset.count(join_vertex_idx) != 0) continue;

        const auto join_vertex = _join_graph->vertices[join_vertex_idx];
        const auto subset_best_tree = _best_tree(vertex_idx_subset);
        if (!subset_best_tree) {
          continue; // Subtree is not connected
        }

        const auto join_tree = _create_join_tree(subset_best_tree, join_vertex);
        if (!join_tree) {
          continue;
        }

        auto joined_vertex_idx_subset = vertex_idx_subset;
        joined_vertex_idx_subset.insert(join_vertex_idx);


        const auto current_best_tree = _best_tree(joined_vertex_idx_subset);

        if (current_best_tree == nullptr || _cost(current_best_tree->lqp) > _cost(join_tree->lqp)) {
          std::cout << "Setting tree for "; for (auto i : vertex_idx_subset) std::cout << i << " "; std::cout << " - joined with " << join_vertex_idx << std::endl;
          _set_best_tree(joined_vertex_idx_subset, join_tree);
        }
      }
    }
  }

  std::set<size_t> all_vertex_ids;
  for (size_t vertex_idx = 0; vertex_idx < num_vertices; ++vertex_idx) all_vertex_ids.insert(vertex_idx);

  return _best_tree(all_vertex_ids)->lqp;
}

std::shared_ptr<const JoinTree> DPLinear::_create_join_tree(const std::shared_ptr<const JoinTree>& tree, const std::shared_ptr<JoinVertex>& join_vertex) const {
  std::vector<std::shared_ptr<JoinEdge>> edges;

  for (const auto& tree_vertex : tree->vertices) {
    auto edge = _join_graph->find_edge(std::make_pair(tree_vertex->node, join_vertex->node));
    if (edge) {
      edges.emplace_back(edge);
    }
  }

  if (edges.empty()) {
    return nullptr;
  }

  std::vector<JoinPredicate> join_predicates;
  for (const auto& edge : edges) {
    std::copy(edge->predicates.begin(), edge->predicates.end(), std::back_inserter(join_predicates));
  }

  std::shared_ptr<AbstractLQPNode> joined_lqp;
  std::shared_ptr<AbstractLQPNode> join_node;

  if (join_predicates.empty()) {
    join_node = std::make_shared<JoinNode>(JoinMode::Cross);
    joined_lqp = join_node;
  } else {
    auto join_predicate = join_predicates[0];

    if (tree->lqp->find_output_column_id_by_column_origin(join_predicate.join_column_origins.first)) {
      Assert(join_vertex->node->find_output_column_id_by_column_origin(join_predicate.join_column_origins.second), "Bug");
    } else {
      Assert(join_vertex->node->find_output_column_id_by_column_origin(join_predicate.join_column_origins.first), "Bug");
      Assert(tree->lqp->find_output_column_id_by_column_origin(join_predicate.join_column_origins.second), "Bug");
      join_predicate = join_predicate.flipped();
    }

    // TODO(moritz) Order edges appropriately
    join_node = std::make_shared<JoinNode>(JoinMode::Inner, join_predicate.join_column_origins, join_predicate.scan_type);
    joined_lqp = join_node;

    for (size_t predicate_idx = 1; predicate_idx < join_predicates.size(); ++predicate_idx) {
      const auto& predicate = join_predicates[predicate_idx];
      auto predicate_node = std::make_shared<PredicateNode>(predicate.join_column_origins.first, predicate.scan_type, predicate.join_column_origins.second);
      predicate_node->set_left_child(joined_lqp);
      joined_lqp = predicate_node;
    }
  }

  join_node->set_left_child(tree->lqp);

  auto right_child = join_vertex->node;
  for (const auto& predicate : join_vertex->predicates) {
    auto predicate_node = std::make_shared<PredicateNode>(predicate.column_origin, predicate.scan_type, predicate.value, predicate.value2);
    predicate_node->set_left_child(right_child);
    right_child = predicate_node;
  }

  join_node->set_right_child(right_child);

  auto vertices = tree->vertices;
  vertices.emplace_back(join_vertex);

  return std::make_shared<JoinTree>(joined_lqp, vertices);
}

std::shared_ptr<const JoinTree> DPLinear::_best_tree(const std::set<size_t>& vertex_ids) const {
  const auto iter = _best_trees.find(vertex_ids);
  return iter == _best_trees.end() ? std::shared_ptr<const JoinTree>{} : iter->second;
}

void DPLinear::_set_best_tree(const std::set<size_t>& vertex_ids, const std::shared_ptr<const JoinTree>& tree) {
  _best_trees[vertex_ids] = tree;
}

float DPLinear::_cost(const std::shared_ptr<AbstractLQPNode>& lqp) const {
  const auto cost_left = lqp->left_child() ? _cost(lqp->left_child()) : 0.0f;
  const auto cost_right = lqp->right_child() ? _cost(lqp->right_child()) : 0.0f;

  auto cost = cost_left + cost_right;

  if (lqp->type() == LQPNodeType::Predicate) {
    cost += lqp->left_child()->get_statistics()->row_count();
  } else if (lqp->type() == LQPNodeType::Join) {
    cost += lqp->left_child()->get_statistics()->row_count() * lqp->right_child()->get_statistics()->row_count();
  } else {
    return lqp->get_statistics()->row_count();
  }

  return cost;
}

}  // namespace opossum

