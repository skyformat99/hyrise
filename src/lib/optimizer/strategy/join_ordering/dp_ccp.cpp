#include "dp_ccp.hpp"

#include <unordered_map>
#include <queue>

#include "enumerate_ccp.hpp"
#include "join_plan_node.hpp"

namespace opossum {

DpCcp::DpCcp(const std::shared_ptr<const JoinGraph>& join_graph):
  AbstractDpAlgorithm(join_graph)
{}

std::shared_ptr<AbstractLQPNode> DpCcp::operator()() {
  _reorder_vertices();

  /**
   * Bring edges into format that EnumerateCcp expects them to be
   */
  std::unordered_map<std::shared_ptr<JoinVertex>, size_t> index_by_vertex;
  for (size_t vertex_idx = 0; vertex_idx < _join_graph->vertices.size(); ++vertex_idx) {
    index_by_vertex[_join_graph->vertices[vertex_idx]] = vertex_idx;
  }

  std::vector<std::pair<size_t, size_t>> enumerate_ccp_edges;
  for (const auto& edge : _join_graph->edges) {
    enumerate_ccp_edges.emplace_back(index_by_vertex[edge->vertices.first], index_by_vertex[edge->vertices.second]);
  }

  /**
   * Initialize best plans for single vertices
   */
  for (size_t vertex_idx = 0; vertex_idx < _join_graph->vertices.size(); ++vertex_idx) {
    boost::dynamic_bitset<> vertex_bit{_join_graph->vertices.size()};
    vertex_bit.set(vertex_idx);

    _set_best_plan(vertex_bit, std::make_shared<JoinPlanNode>(_join_graph->vertices[vertex_idx]));
  }

  /**
   * Actual DpCcp algorithm
   */
  const auto csg_cmp_pairs = EnumerateCcp{_join_graph->vertices.size(), enumerate_ccp_edges}();
  for (const auto& csg_cmp_pair : csg_cmp_pairs) {
    const auto best_plan_left = _get_best_plan(csg_cmp_pair.first);
    const auto best_plan_right = _get_best_plan(csg_cmp_pair.second);
    const auto current_plan = _create_join_plan(best_plan_left, best_plan_right);
    const auto current_best_plan = _get_best_plan(csg_cmp_pair.first | csg_cmp_pair.second);

    if (!current_best_plan || current_plan->cost() < current_best_plan->cost()) {
      _set_best_plan(csg_cmp_pair.first | csg_cmp_pair.second, current_plan);
    }
  }

  boost::dynamic_bitset<> all_vertices_set{_join_graph->vertices.size()};
  all_vertices_set.flip(); // Turns all bits to '1'

  return _get_best_plan(all_vertices_set)->to_lqp();
}

void DpCcp::_reorder_vertices() {
  if (_join_graph->vertices.empty()) return;

  /**
   * Breadth first flooding of the JoinGraph, storing the order in which vertices are discovered in `new_index_by_vertex`
   */
  std::unordered_map<std::shared_ptr<JoinVertex>, size_t> new_index_by_vertex;
  std::queue<std::shared_ptr<JoinVertex>> vertex_queue;

  vertex_queue.push(_join_graph->vertices.front());

  auto current_index = size_t{0};

  while (!vertex_queue.empty()) {
    auto vertex = vertex_queue.front();
    vertex_queue.pop();

    new_index_by_vertex.emplace(vertex, current_index);
    ++current_index;

    const auto edges = _join_graph->find_edges(vertex->node);
    for (const auto& edge : edges) {
      auto adjacent_vertex = edge->get_adjacent_vertex(vertex->node);
      if (new_index_by_vertex.find(adjacent_vertex) == new_index_by_vertex.end()) {
        vertex_queue.push(adjacent_vertex);
      }
    }
  }

  /**
   * Build reordered vertex vector
   */
  JoinGraph::Vertices ordered_vertices(_join_graph->vertices.size());
  for (const auto& vertex_index_pair : new_index_by_vertex) {
    ordered_vertices[vertex_index_pair.second] = vertex_index_pair.first;
  }

  _join_graph->vertices = ordered_vertices;
}

}  // namespace opossum