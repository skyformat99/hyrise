#include "join_graph.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "constant_mappings.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

LQPJoinPredicate::LQPJoinPredicate(const JoinLQPNodes& join_nodes, const JoinMode join_mode):
  join_nodes(join_nodes),
  join_mode(join_mode)
{
  DebugAssert(join_mode == JoinMode::Cross, "This constructor only supports cross join edges");
}

LQPJoinPredicate::LQPJoinPredicate(const JoinColumnOrigins& join_column_origins, JoinMode join_mode, ScanType scan_type):
  join_nodes(join_column_origins.first.node(), join_column_origins.second.node()),
  join_column_origins(join_column_origins),
  join_mode(join_mode),
  scan_type(scan_type)
{
  DebugAssert(join_mode == JoinMode::Inner, "This constructor only supports inner join edges");
}

std::string LQPJoinPredicate::description(DescriptionMode description_mode) const {
  if (join_mode == JoinMode::Cross) {
    return "CrossJoin";
  }

  const auto separator = description_mode == DescriptionMode::SingleLine ? " " : "\\n";
  const auto scan_type_name = scan_type_to_string.left.at(*scan_type);

  return join_column_origins->first.description() + separator + scan_type_name + separator + join_column_origins->second.description();
}

LQPNodePredicate::LQPNodePredicate(const LQPColumnOrigin& column_origin,
                 const ScanType scan_type,
                 const AllParameterVariant& value,
                 const std::optional<AllTypeVariant>& value2) {

}

std::string LQPNodePredicate::description() const {
  return "";
//  return node->get_qualified_column_name(vertex_predicate.column_id) + " " +
//         scan_type_to_string.left.at(vertex_predicate.scan_type) + " " + boost::lexical_cast<std::string>(vertex_predicate.value);
}


JoinVertex::JoinVertex(const std::shared_ptr<AbstractLQPNode>& node):
  node(node)
{

}

std::string JoinVertex::description() const {
  return "";
//  std::stringstream stream;
//  stream << node->description();
//
//  if (!predicates.empty()) {
//    stream << "[";
//    for (size_t predicate_idx = 0; predicate_idx < predicates.size(); ++predicate_idx) {
//      stream << get_predicate_description(predicates[predicate_idx]);
//      if (predicate_idx + 1 < predicates.size()) {
//        stream << ", ";
//      }
//    }
//    stream << "]";
//  }
//
//  return stream.str();
}

JoinGraph::JoinGraph(Vertices vertices, JoinPredicates predicates):
  _vertices(std::move(vertices)),
  _predicates(std::move(predicates))
{

}

const JoinGraph::Vertices& JoinGraph::vertices() const {
  return _vertices;
}

const JoinGraph::JoinPredicates& JoinGraph::join_predicates() const {
  return _predicates;
}

void JoinGraph::print(std::ostream& out) const {
//  out << "==== JoinGraph ====" << std::endl;
//  out << "==== Vertices ====" << std::endl;
//  for (size_t vertex_idx = 0; vertex_idx < _vertices.size(); ++vertex_idx) {
//    const auto& vertex = _vertices[vertex_idx];
//    std::cout << vertex_idx << ":  " << vertex.description() << std::endl;
//  }
//  out << "==== Edges ====" << std::endl;
//  for (const auto& predicate : _predicates) {
//    if (predicate.join_mode == JoinMode::Inner) {
//      std::cout << edge.vertex_ids.first << " <-- " << get_edge_description(edge) << " --> "
//                << edge.vertex_ids.second << std::endl;
//    } else {
//      std::cout << edge.vertex_ids.first << " <----> " << edge.vertex_ids.second << std::endl;
//    }
//  }

//  out << "===================" << std::endl;
}

} // namespace opossum