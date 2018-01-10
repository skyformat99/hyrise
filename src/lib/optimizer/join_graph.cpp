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

JoinVertex::JoinVertex(const std::shared_ptr<const AbstractLQPNode>& node): node(node) {

}

JoinEdge::JoinEdge(const JoinVertexPair& vertices): vertices(std::move(vertices)) {}

JoinGraph::JoinGraph(Vertices vertices, Edges edges):
  vertices(vertices), edges(edges) {}

} // namespace opossum