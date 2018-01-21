#pragma once

#include "abstract_dp_algorithm.hpp"

namespace opossum {

class DpCcp : public AbstractDpAlgorithm {
 public:
  explicit DpCcp(const std::shared_ptr<const JoinGraph>& join_graph);

  std::shared_ptr<AbstractLQPNode> operator()() override;

 private:
  void _reorder_vertices();
};

}  // namespace opossum
