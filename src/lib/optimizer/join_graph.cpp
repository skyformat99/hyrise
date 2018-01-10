#include "join_graph.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "constant_mappings.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

JoinPredicate::JoinPredicate(const JoinMode join_mode): join_mode(join_mode) {
  DebugAssert(join_mode == JoinMode::Cross, "This constructor only supports cross join edges");
}

JoinPredicate::JoinPredicate(JoinMode join_mode, const JoinColumnOrigins& join_column_origins, ScanType scan_type):
  join_mode(join_mode),
  join_column_origins(join_column_origins),
  scan_type(scan_type)
{
  DebugAssert(join_mode == JoinMode::Inner, "This constructor only supports inner join edges");
}

JoinVertexPredicate::JoinVertexPredicate(const LQPColumnOrigin& column_origin,
                    const ScanType scan_type,
                    const AllParameterVariant& value,
                    const std::optional<AllTypeVariant>& value2):
  column_origin(column_origin), scan_type(scan_type), value(value), value2(value2)
{

}

JoinVertex::JoinVertex(const std::shared_ptr<const AbstractLQPNode>& node): node(node) {

}

JoinEdge::JoinEdge(Vertices vertices, JoinPredicates predicates): vertices(std::move(vertices)), predicates(std::move(predicates)) {}

JoinGraph::JoinGraph(Vertices vertices, Edges edges):
  vertices(vertices), edges(edges) {}

} // namespace opossum