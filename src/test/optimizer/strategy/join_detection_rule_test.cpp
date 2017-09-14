  #include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "optimizer/abstract_syntax_tree/abstract_ast_node.hpp"
#include "optimizer/abstract_syntax_tree/join_node.hpp"
#include "optimizer/abstract_syntax_tree/predicate_node.hpp"
#include "optimizer/abstract_syntax_tree/projection_node.hpp"
#include "optimizer/abstract_syntax_tree/stored_table_node.hpp"
#include "optimizer/expression.hpp"
#include "optimizer/strategy/join_detection_rule.hpp"
#include "sql/sql_to_ast_translator.hpp"
#include "storage/storage_manager.hpp"

namespace opossum {

class JoinDetectionRuleTest : public BaseTest {
 protected:
  void SetUp() override {
    StorageManager::get().add_table("a", load_table("src/test/tables/int_float.tbl", 2));
    StorageManager::get().add_table("b", load_table("src/test/tables/int_float.tbl", 2));
    StorageManager::get().add_table("c", load_table("src/test/tables/int_float.tbl", 2));

    _table_node_a = std::make_shared<StoredTableNode>("a");
    _table_node_b = std::make_shared<StoredTableNode>("b");
    _table_node_c = std::make_shared<StoredTableNode>("c");
  }

  std::shared_ptr<StoredTableNode> _table_node_a, _table_node_b, _table_node_c;
  JoinConditionDetectionRule _rule;
};

TEST_F(JoinDetectionRuleTest, SimpleDetectionTest) {
  /**
   * Test that
   *
   *   Predicate
   *  (a.a == b.b)
   *       |
   *     Cross
   *    /     \
   *   a      b
   *
   * gets converted to
   *
   *      Join
   *  (a.a == b.b)
   *    /     \
   *   a      b
   */

  // Generate AST
  const auto cross_join_node = std::make_shared<JoinNode>(JoinMode::Cross);
  cross_join_node->set_left_child(_table_node_a);
  cross_join_node->set_right_child(_table_node_b);

  const auto predicate_node = std::make_shared<PredicateNode>(ColumnID{0}, ScanType::OpEquals, ColumnID{3});
  predicate_node->set_left_child(cross_join_node);

  auto output = _rule.apply_to(predicate_node);

  EXPECT_EQ(output->type(), ASTNodeType::Join);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, SecondDetectionTest) {
  /**
   * Test that
   *
   *   Projection
   *     (a.a)
   *       |
   *   Predicate
   *  (a.a == b.b)
   *       |
   *     Cross
   *    /     \
   *   a      b
   *
   * gets converted to
   *
   *   Projection
   *     (a.a)
   *       |
   *      Join
   *  (a.a == b.b)
   *    /     \
   *   a      b
   */

  // Generate AST
  const auto cross_join_node = std::make_shared<JoinNode>(JoinMode::Cross);
  cross_join_node->set_left_child(_table_node_a);
  cross_join_node->set_right_child(_table_node_b);

  const auto predicate_node = std::make_shared<PredicateNode>(ColumnID{0}, ScanType::OpEquals, ColumnID{3});
  predicate_node->set_left_child(cross_join_node);

  const std::vector<std::shared_ptr<Expression>> columns = {Expression::create_column(ColumnID{0})};
  const auto projection_node = std::make_shared<ProjectionNode>(columns);
  projection_node->set_left_child(predicate_node);

  auto output = _rule.apply_to(projection_node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Join);
  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, NoPredicate) {
  /**
   * Test that
   *
   *   Projection
   *     (a.a)
   *       |
   *     Cross
   *    /     \
   *   a      b
   *
   * is not manipulated
   */

  // Generate AST
  const auto cross_join_node = std::make_shared<JoinNode>(JoinMode::Cross);
  cross_join_node->set_left_child(_table_node_a);
  cross_join_node->set_right_child(_table_node_b);

  const std::vector<std::shared_ptr<Expression>> columns = {Expression::create_column(ColumnID{0})};
  const auto projection_node = std::make_shared<ProjectionNode>(columns);
  projection_node->set_left_child(cross_join_node);

  auto output = _rule.apply_to(projection_node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Join);
  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, NoMatchingPredicate) {
  /**
   * Test that
   *
   *   Projection
   *     (a.a)
   *       |
   *   Predicate
   *  (a.a == a.b)
   *       |
   *     Cross
   *    /     \
   *   a      b
   *
   * isn't manipulated.
   */

  // Generate AST
  const auto cross_join_node = std::make_shared<JoinNode>(JoinMode::Cross);
  cross_join_node->set_left_child(_table_node_a);
  cross_join_node->set_right_child(_table_node_b);

  const auto predicate_node = std::make_shared<PredicateNode>(ColumnID{0}, ScanType::OpEquals, ColumnID{1});
  predicate_node->set_left_child(cross_join_node);

  const std::vector<std::shared_ptr<Expression>> columns = {Expression::create_column(ColumnID{0})};
  const auto projection_node = std::make_shared<ProjectionNode>(columns);
  projection_node->set_left_child(predicate_node);

  auto output = _rule.apply_to(projection_node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Predicate);
  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::Join);
  EXPECT_EQ(output->left_child()->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->left_child()->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, NonCrossJoin) {
  /**
   * Test that
   *
   *   Projection
   *     (a.a)
   *       |
   *   Predicate
   *  (a.a == a.b)
   *       |
   *     Join
   *  (a.b == b.b)
   *    /     \
   *   a      b
   *
   * isn't manipulated.
   */


  const auto join_node =
      std::make_shared<JoinNode>(JoinMode::Inner, std::make_pair(ColumnID{1}, ColumnID{3}), ScanType::OpEquals);
  join_node->set_left_child(_table_node_a);
  join_node->set_right_child(_table_node_b);

  const auto predicate_node = std::make_shared<PredicateNode>(ColumnID{0}, ScanType::OpEquals, ColumnID{3});
  predicate_node->set_left_child(join_node);

  const std::vector<std::shared_ptr<Expression>> columns = {Expression::create_column(ColumnID{0})};
  const auto projection_node = std::make_shared<ProjectionNode>(columns);
  projection_node->set_left_child(predicate_node);

  auto output = _rule.apply_to(projection_node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Predicate);
  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::Join);
  EXPECT_EQ(output->left_child()->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->left_child()->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, MultipleJoins) {  /**
   * Test that
   *
   *       Projection
   *         (a.a)
   *           |
   *        Predicate
   *      (a.a == b.a)
   *            |
   *          Cross
   *         /     \
   *     Cross     c
   *    /     \
   *   a      b
   *
   *   gets converted to
   *
   *       Projection
   *         (a.a)
   *            |
   *          Cross
   *         /     \
   *      Join     c
   *  (a.a == b.a)
   *    /     \
   *   a      b
   *
   */
  const auto join_node1 = std::make_shared<JoinNode>(JoinMode::Cross);
  join_node1->set_left_child(_table_node_a);
  join_node1->set_right_child(_table_node_b);

  const auto join_node2 = std::make_shared<JoinNode>(JoinMode::Cross);
  join_node2->set_left_child(join_node1);
  join_node2->set_right_child(_table_node_c);

  const auto predicate_node = std::make_shared<PredicateNode>(ColumnID{0}, ScanType::OpEquals, ColumnID{2});
  predicate_node->set_left_child(join_node2);

  const std::vector<std::shared_ptr<Expression>> columns = {Expression::create_column(ColumnID{0})};
  const auto projection_node = std::make_shared<ProjectionNode>(columns);
  projection_node->set_left_child(predicate_node);

  auto output = _rule.apply_to(projection_node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Join);

  const auto first_join_node = std::dynamic_pointer_cast<JoinNode>(output->left_child());
  EXPECT_EQ(first_join_node->join_mode(), JoinMode::Cross);

  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::Join);
  const auto second_join_node = std::dynamic_pointer_cast<JoinNode>(output->left_child()->left_child());
  EXPECT_EQ(second_join_node->join_mode(), JoinMode::Inner);

  EXPECT_EQ(output->left_child()->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->left_child()->right_child()->type(), ASTNodeType::StoredTable);
}

TEST_F(JoinDetectionRuleTest, MultipleJoinsSQL) {
  hsql::SQLParserResult parse_result;
  hsql::SQLParser::parseSQLString("SELECT * FROM a, b, c WHERE a.a = b.a", &parse_result);

  auto node = SQLToASTTranslator::get().translate_parse_result(parse_result)[0];
  auto output = _rule.apply_to(node);

  EXPECT_EQ(output->type(), ASTNodeType::Projection);
  EXPECT_EQ(output->left_child()->type(), ASTNodeType::Join);

  const auto first_join_node = std::dynamic_pointer_cast<JoinNode>(output->left_child());
  EXPECT_EQ(first_join_node->join_mode(), JoinMode::Inner);

  const auto first_table = std::dynamic_pointer_cast<StoredTableNode>(output->left_child()->right_child());
  EXPECT_EQ(first_table->table_name(), "a");

  EXPECT_EQ(output->left_child()->left_child()->type(), ASTNodeType::Join);
  const auto second_join_node = std::dynamic_pointer_cast<JoinNode>(output->left_child()->left_child());
  EXPECT_EQ(second_join_node->join_mode(), JoinMode::Cross);

  EXPECT_EQ(output->left_child()->left_child()->left_child()->type(), ASTNodeType::StoredTable);
  EXPECT_EQ(output->left_child()->left_child()->right_child()->type(), ASTNodeType::StoredTable);

  const auto second_table =
      std::dynamic_pointer_cast<StoredTableNode>(output->left_child()->left_child()->left_child());
  EXPECT_EQ(second_table->table_name(), "b");
  const auto third_table =
      std::dynamic_pointer_cast<StoredTableNode>(output->left_child()->left_child()->right_child());
  EXPECT_EQ(third_table->table_name(), "c");
}

}  // namespace opossum
