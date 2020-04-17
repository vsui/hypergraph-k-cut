//
// Created by Victor on 4/16/20.
//

#include "sqlutil.hpp"

#include <algorithm>

sqlutil::InsertStatementBuilder::InsertStatementBuilder(std::string table_name) : table_name_(std::move(table_name)) {}

void sqlutil::InsertStatementBuilder::add(const std::string &col_name, const std::string &val) {
  col_vals_.emplace_back(col_name, _impl::sql_val_str(val));
}

void sqlutil::InsertStatementBuilder::add(const std::string &col_name, const int val) {
  col_vals_.emplace_back(col_name, _impl::sql_val_str(val));
}

std::string sqlutil::InsertStatementBuilder::string() {
  std::vector<std::string> names;
  std::vector<std::string> vals;

  std::transform(col_vals_.cbegin(),
                 col_vals_.cend(),
                 std::back_inserter(names),
                 [](const auto &a) { return std::get<0>(a); });
  std::transform(col_vals_.cbegin(),
                 col_vals_.cend(),
                 std::back_inserter(vals),
                 [](const auto &a) { return std::get<1>(a); });

  std::string names_str = _impl::comma_delimit(std::begin(names), std::end(names));
  std::string vals_str = _impl::comma_delimit(std::begin(vals), std::end(vals));

  std::ostringstream oss;
  oss << "INSERT INTO " << table_name_ << " (" << names_str << ") VALUES (" << vals_str << ")";

  return oss.str();
}
