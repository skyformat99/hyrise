#include "join_graph.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "constant_mappings.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

JoinPredicate::JoinPredicate(JoinMode join_mode, const JoinColumnOrigins& join_column_origins, ScanType scan_type):
  join_mode(join_mode),
  join_column_origins(join_column_origins),
  scan_type(scan_type)
{
  DebugAssert(join_mode == JoinMode::Inner, "This constructor only supports inner join edges");
}

void JoinPredicate::print(std::ostream& stream) const {
  stream << "{[" << join_mode_to_string.at(join_mode) << "]";
  stream << join_column_origins.first.description() << " " << scan_type_to_string.left.at(scan_type) << " " << join_column_origins.second.description() << "}";
}

bool JoinPredicate::operator==(const JoinPredicate& rhs) const {
  return join_mode == rhs.join_mode && join_column_origins == rhs.join_column_origins && scan_type == rhs.scan_type;
}

JoinVertex::JoinVertex(const std::shared_ptr<const AbstractLQPNode>& node): node(node) {

}

void JoinVertex::print(std::ostream& stream) const {
  stream << "Vertex of ";
  stream << node->description() << "@" << node.get() << ", " << predicates.size() << " predicates" << std::endl;
  for (const auto& predicate : predicates) {
    predicate.print(stream);
    stream << std::endl;
  }
}

JoinEdge::JoinEdge(const JoinVertexPair& vertices): vertices(std::move(vertices)) {}

void JoinEdge::print(std::ostream& stream) const {
  stream << "Edge between " << vertices.first->node.get() << " and " << vertices.second->node.get() << ", " << predicates.size() << " predicates" << std::endl;
  for (const auto& predicate : predicates) {
    predicate.print(stream);
    stream << std::endl;
  }
}

JoinGraph::JoinGraph(Vertices vertices, Edges edges):
  vertices(vertices), edges(edges) {}

std::shared_ptr<JoinEdge> JoinGraph::find_edge(const std::pair<std::shared_ptr<AbstractLQPNode>, std::shared_ptr<AbstractLQPNode>>& nodes) const {
  const auto iter = std::find_if(edges.begin(), edges.end(), [&](const auto& edge) {
    return (edge->vertices.first->node == nodes.first && edge->vertices.second->node == nodes.second) ||
    (edge->vertices.first->node == nodes.second && edge->vertices.second->node == nodes.first);
  });
  return iter == edges.end() ? nullptr : *iter;
}

void JoinGraph::print(std::ostream& stream) const {
  stream << "==== Vertices ====" << std::endl;
  if (vertices.empty()) {
    stream << "<none>" << std::endl;
  } else {
    for (const auto &vertex : vertices) {
      vertex->print(stream);
    }
  }
  stream << "===== Edges ======" << std::endl;
  if (edges.empty()) {
    stream << "<none>" << std::endl;
  } else {
    for (const auto& edge : edges) {
      edge->print(stream);
    }
  }
  std::cout << std::endl;

}

} // namespace opossum