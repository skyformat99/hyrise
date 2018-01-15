#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace opossum {

class AbstractLQPNode;
class JoinGraph;
class JoinVertex;

struct JoinTree {
  JoinTree(const std::shared_ptr<AbstractLQPNode>& lqp, const std::vector<std::shared_ptr<JoinVertex>>& vertices): lqp(lqp), vertices(vertices) {

  }

  std::shared_ptr<AbstractLQPNode> lqp;
  std::vector<std::shared_ptr<JoinVertex>> vertices;
};

class DPLinear {
 public:
  explicit DPLinear(const std::shared_ptr<const JoinGraph>& join_graph);

  std::shared_ptr<AbstractLQPNode> run();

 private:
  std::shared_ptr<const JoinTree> _create_join_tree(const std::shared_ptr<const JoinTree>& tree, const std::shared_ptr<JoinVertex>& join_vertex) const;
  std::shared_ptr<const JoinTree> _best_tree(const std::set<size_t>& vertex_ids) const;
  void _set_best_tree(const std::set<size_t>& vertex_ids, const std::shared_ptr<const JoinTree>& tree);
  float _cost(const std::shared_ptr<AbstractLQPNode>& lqp) const;

  std::shared_ptr<const JoinGraph> _join_graph;
  std::map<std::set<size_t>, std::shared_ptr<const JoinTree>> _best_trees;
};

}  // namespace opossum