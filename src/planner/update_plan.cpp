//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// drop_plan.cpp
//
// Identification: src/planner/drop_plan.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "planner/update_plan.h"

#include "planner/project_info.h"
#include "common/types.h"

#include "storage/data_table.h"
#include "catalog/bootstrapper.h"
#include "parser/statement_update.h"
#include "parser/table_ref.h"
#include "expression/expression_util.h"

namespace peloton {
namespace planner {

UpdatePlan::UpdatePlan(storage::DataTable *table,
                       std::unique_ptr<const planner::ProjectInfo> project_info)
    : target_table_(table),
      project_info_(std::move(project_info)),
      updates(NULL),
      where(NULL) { }

UpdatePlan::UpdatePlan(parser::UpdateStatement *parse_tree) {

  updates = new std::vector<parser::UpdateClause *>();
  auto t_ref = parse_tree->table;
  table_name = std::string(t_ref->name);
  target_table_ = catalog::Bootstrapper::global_catalog->GetTableFromDatabase(
      DEFAULT_DB_NAME, table_name);

  for(auto update_clause : *parse_tree->updates) {
	  updates->push_back(update_clause->Copy());
  }
  TargetList tlist;
  DirectMapList dmlist;
  oid_t col_id;
  auto schema = target_table_->GetSchema();

  std::vector<oid_t> columns;
  for (auto update : *updates) {
    // get oid_t of the column and push it to the vector;
    col_id = schema->GetColumnID(std::string(update->column));
    columns.push_back(col_id);
    tlist.emplace_back(col_id, update->value->Copy());
  }

  for (uint i = 0; i < schema->GetColumns().size(); i++) {
    if (schema->GetColumns()[i].column_name !=
        schema->GetColumns()[col_id].column_name)
      dmlist.emplace_back(i, std::pair<oid_t, oid_t>(0, i));
  }

  std::unique_ptr<const planner::ProjectInfo> project_info(
      new planner::ProjectInfo(std::move(tlist), std::move(dmlist)));
  project_info_ = std::move(project_info);

  where = parse_tree->where->Copy();
  ReplaceColumnExpressions(target_table_->GetSchema(), where);

  std::unique_ptr<planner::SeqScanPlan> seq_scan_node(
      new planner::SeqScanPlan(target_table_, where, columns));
  AddChild(std::move(seq_scan_node));
}

void UpdatePlan::SetParameterValues(std::vector<Value> *values) {
  LOG_INFO("Setting values for parameters in updates");
  // First update project_info_ target list
  // Create new project_info_
  TargetList tlist;
  DirectMapList dmlist;
  oid_t col_id;
  auto schema = target_table_->GetSchema();

  std::vector<oid_t> columns;
  for (auto update : *updates) {
    // get oid_t of the column and push it to the vector;
    col_id = schema->GetColumnID(std::string(update->column));
    columns.push_back(col_id);
    tlist.emplace_back(col_id, update->value->Copy());
  }

  for (uint i = 0; i < schema->GetColumns().size(); i++) {
    if (schema->GetColumns()[i].column_name !=
        schema->GetColumns()[col_id].column_name)
      dmlist.emplace_back(i, std::pair<oid_t, oid_t>(0, i));
  }

  auto new_proj_info = new planner::ProjectInfo(std::move(tlist), std::move(dmlist));
  new_proj_info->transformParameterToConstantValueExpression(values);
  project_info_.reset(new_proj_info);

  for(auto update_expr : *updates) {
	  // The assignment parameter is an expression with left and right
	  if(update_expr->value->GetLeft() && update_expr->value->GetRight()) {
		  expression::ExpressionUtil::ConvertParameterExpressions(update_expr->value, values);
	  }
	  // The assignment parameter is a single value
	  else {
		  auto param_expr = (expression::ParameterValueExpression*) update_expr->value;
		  LOG_INFO("Setting parameter %u to value %s", param_expr->getValueIdx(),
				  values->at(param_expr->getValueIdx()).GetInfo().c_str());
		  auto value = new expression::ConstantValueExpression(values->at(param_expr->getValueIdx()));
		  delete param_expr;
		  update_expr->value = value;
	  }
  }
  LOG_INFO("Setting values for parameters in where");
  expression::ExpressionUtil::ConvertParameterExpressions(where, values);
}

}  // namespace planner
}  // namespace peloton
