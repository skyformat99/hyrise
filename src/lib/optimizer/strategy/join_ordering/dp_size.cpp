//#include "dp_size.hpp"
//
//#include "optimizer/join_graph.hpp"
//
//namespace opossum {
//
//DPSize::DPSize(const std::shared_ptr<const JoinGraph>& join_graph):
//  _join_graph(join_graph)
//{}
//
//std::shared_ptr<AbstractLQPNode> DPSize::run() {
//  for (size_t vertex_idx = 0; vertex_idx < num_vertices; ++vertex_idx) {
//    std::set<size_t> single_vertex_idx;
//    single_vertex_idx.insert(vertex_idx);
//
//    const auto single_vertex_tree = std::make_shared<JoinTree>(_join_graph->vertices[vertex_idx]->node, std::vector<std::shared_ptr<JoinVertex>>{_join_graph->vertices[vertex_idx]});
//
//    _set_best_tree(single_vertex_idx, single_vertex_tree);
//  }
//}
//
//}  // namespace opossum
