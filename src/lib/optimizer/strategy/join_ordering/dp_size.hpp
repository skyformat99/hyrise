//#pragma once
//
//#include <memory>
//
//namespace opossum {
//
//class AbstractLQPNode;
//class JoinGraph;
//
//class DPSize final {
// public:
//  explicit DPSize(const std::shared_ptr<const JoinGraph>& join_graph);
//
//  std::shared_ptr<AbstractLQPNode> run();
//
// private:
//  std::shared_ptr<const JoinGraph> _join_graph;
//};
//
//}