#include "projection_node.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "lqp_expression.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

ProjectionNode::ProjectionNode(const std::vector<std::shared_ptr<LQPExpression>>& column_expressions)
    : AbstractLQPNode(LQPNodeType::Projection), _column_expressions(column_expressions) {}

std::string ProjectionNode::description() const {
  std::ostringstream desc;

  desc << "[Projection] ";

  std::vector<std::string> verbose_column_names;
  if (left_child()) {
    verbose_column_names = left_child()->get_verbose_column_names();
  }

  for (size_t column_idx = 0; column_idx < _column_expressions.size(); ++column_idx) {
    desc << _column_expressions[column_idx]->to_string(verbose_column_names);
    if (column_idx + 1 < _column_expressions.size()) {
      desc << ", ";
    }
  }

  return desc.str();
}

const std::vector<std::shared_ptr<LQPExpression>>& ProjectionNode::column_expressions() const {
  return _column_expressions;
}

void ProjectionNode::_on_child_changed() {
  DebugAssert(!right_child(), "Projection can't have a right child");

  _output_column_names.reset();
}

const std::vector<std::optional<ColumnID>>& ProjectionNode::output_column_ids_to_input_column_ids() const {
  if (!_output_column_ids_to_input_column_ids) {
    _update_output();
  }

  return *_output_column_ids_to_input_column_ids;
}

const std::vector<std::string>& ProjectionNode::output_column_names() const {
  if (!_output_column_names) {
    _update_output();
  }
  return *_output_column_names;
}

std::string ProjectionNode::get_verbose_column_name(ColumnID column_id) const {
  DebugAssert(left_child(), "Need input to generate name");
  DebugAssert(column_id < _column_expressions.size(), "ColumnID out of range");

  const auto& column_expression = _column_expressions[column_id];

  if (column_expression->alias()) {
    return *column_expression->alias();
  }

  if (left_child()) {
    return column_expression->to_string(left_child()->output_column_names());
  } else {
    return column_expression->to_string();
  }
}

void ProjectionNode::_update_output() const {
  /**
   * The output (column names and output-to-input mapping) of this node gets cleared whenever a child changed and is
   * re-computed on request. This allows LQPs to be in temporary invalid states (e.g. no left child in Join) and thus
   * allows easier manipulation in the optimizer.
   */

  DebugAssert(!_output_column_ids_to_input_column_ids, "No need to update, _update_output() shouldn't get called.");
  DebugAssert(!_output_column_names, "No need to update, _update_output() shouldn't get called.");
  DebugAssert(left_child(), "Can't set output without input");

  _output_column_names.emplace();
  _output_column_names->reserve(_column_expressions.size());

  _output_column_ids_to_input_column_ids.emplace();
  _output_column_ids_to_input_column_ids->reserve(_column_expressions.size());

  for (const auto& expression : _column_expressions) {
    // If the expression defines an alias, use it as the output column name.
    // If it does not, we have to handle it differently, depending on the type of the expression.
    if (expression->alias()) {
      _output_column_names->emplace_back(*expression->alias());
    }

    if (expression->type() == ExpressionType::Column) {
      DebugAssert(left_child(), "ProjectionNode needs a child.");

      const auto input_column_id = left_child()->find_output_column_id_by_column_origin(expression->column_origin());
      _output_column_ids_to_input_column_ids->emplace_back(input_column_id);

      if (!expression->alias()) {
        Assert(input_column_id < left_child()->output_column_names().size(), "ColumnID out of range");
        const auto& column_name = left_child()->output_column_names()[input_column_id];
        _output_column_names->emplace_back(column_name);
      }

    } else if (expression->type() == ExpressionType::Literal || expression->is_arithmetic_operator()) {
      _output_column_ids_to_input_column_ids->emplace_back(std::nullopt);

      if (!expression->alias()) {
        _output_column_names->emplace_back(expression->to_string(left_child()->output_column_names()));
      }

    } else {
      Fail("Only column references, arithmetic expressions, and literals supported for now.");
    }
  }
}

}  // namespace opossum
