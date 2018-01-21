#include "join_plan_node.hpp"

namespace opossum {

AbstractJoinPlanNode::AbstractJoinPlanNode(const JoinPlanNodeType type):
  _type(type)
{}

JoinPlanNodeType AbstractJoinPlanNode::type() const {
  return _type;
}

float AbstractJoinPlanNode::cost() const {
  return _cost;
}

JoinPlanVertexNode::JoinPlanVertexNode(const std::shared_ptr<JoinVertex>& join_vertex):
AbstractJoinPlanNode(JoinPlanNodeType::Vertex),
  _join_vertex(join_vertex)
{}

std::shared_ptr<AbstractLQPNode> JoinPlanVertexNode::to_lqp() const {

}

JoinPlanEdgeNode::JoinPlanEdgeNode(const std::shared_ptr<AbstractJoinPlanNode>& left_child, const std::shared_ptr<AbstractJoinPlanNode>& right_child,
                 const std::shared_ptr<JoinEdge>& join_edge):
AbstractJoinPlanNode(JoinPlanNodeType::Edge),
_left_child(left_child),
_right_child(right_child),
_join_edge(join_edge)
{}

std::shared_ptr<AbstractLQPNode> JoinPlanEdgeNode::to_lqp() const {
  
}


}  // namespace opossum
