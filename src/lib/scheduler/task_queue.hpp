#pragma once

#include <stdint.h>
#include <tbb/concurrent_queue.h>
#include <array>
#include <atomic>
#include <memory>

#include "types.hpp"

namespace opossum {

class AbstractTask;

/**
 * Holds a queue of AbstractTasks, usually one of these exists per node
 */
class TaskQueue {
 public:
  static constexpr uint32_t NUM_PRIORITY_LEVELS = 3;

  explicit TaskQueue(NodeID node_id);

  bool empty() const;

  NodeID node_id() const;

  void push(std::shared_ptr<AbstractTask> task, uint32_t priority);

  /**
   * Returns a Tasks that is ready to be executed and removes it from the queue
   */
  std::shared_ptr<AbstractTask> pull();

  /**
   * Returns a Tasks that is ready to be executed and removes it from one of the stealable queues
   */
  std::shared_ptr<AbstractTask> steal();

 private:
  NodeID _node_id;
  std::array<tbb::concurrent_queue<std::shared_ptr<AbstractTask>>, NUM_PRIORITY_LEVELS> _queues;
  std::atomic_uint _num_tasks{0};
};

}  // namespace opossum
