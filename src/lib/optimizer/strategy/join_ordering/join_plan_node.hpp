#pragma once

#include <memory>

namespace opossum {

class AbstractLQPNode;
class JoinVertex;
class JoinEdge;

enum class JoinPlanNodeType { Vertex, Edge };

class AbstractJoinPlanNode {
 public:
  explicit AbstractJoinPlanNode(const JoinPlanNodeType type);
  virtual ~AbstractJoinPlanNode() = default;

  JoinPlanNodeType type() const;
  float cost() const;

  virtual std::shared_ptr<AbstractLQPNode> to_lqp() const = 0;

 protected:
  const JoinPlanNodeType _type;
  float _cost{0.0f};
};

class JoinPlanVertexNode final: public AbstractJoinPlanNode {
 public:
  JoinPlanVertexNode(const std::shared_ptr<JoinVertex>& join_vertex);

  std::shared_ptr<AbstractLQPNode> to_lqp() const override;

 private:
  const std::shared_ptr<JoinVertex> _join_vertex;
};

class JoinPlanEdgeNode final : public AbstractJoinPlanNode {
 public:
  JoinPlanEdgeNode(const std::shared_ptr<AbstractJoinPlanNode>& left_child, const std::shared_ptr<AbstractJoinPlanNode>& right_child,
                   const std::shared_ptr<JoinEdge>& join_edge);

  std::shared_ptr<AbstractLQPNode> to_lqp() const override;

 private:
  const std::shared_ptr<const AbstractJoinPlanNode> _left_child;
  const std::shared_ptr<const AbstractJoinPlanNode> _right_child;
  const std::shared_ptr<const JoinEdge> _join_edge;
  float _cost;
};
}  // namespace opossum
