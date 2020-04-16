//
// Created by Victor on 3/7/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP

#include <string>
#include <sstream>
#include <tuple>
#include <vector>

namespace sqlutil {

/* Struct representing `time("now")` SQL function */
struct TimeNow {};

namespace _impl {
template<typename InputIt>

// Delimit a range of strings with ", "'s
std::string comma_delimit(InputIt begin, InputIt end) {
  std::ostringstream oss;
  oss << *begin;
  for (auto it = ++begin; it != end; ++it) {
    oss << ", " << *it;
  }
  return oss.str();
}

inline std::string sql_val_str(int val) {
  return std::to_string(val);
}

inline std::string sql_val_str(const std::string &val) {
  return "'" + val + "'";
}

inline std::string sql_val_str(TimeNow) {
  return "time('now')";
}

} // namespace _impl

/**
 * Create a SQL insert statement out of tuples of column names and values
 *
 * Example usage:
 *
 * ```
 * auto mit = std::make_tuple<std::string, int>;
 * auto mst = std::make_tuple<std::string, std::string>;
 *
 * std::string stmt = insert_statement(
 *  "runs",
 *  mit("num_runs", 1001),
 *  mst("hypergraph_id", "randomring.hgr")
 * );
 *
 * // INSERT INTO runs (num_runs, hypergraph_id) VALUES (1001, 'randomring.hgr')
 *
 * ```
 */
template<typename ...Ts>
std::string insert_statement(const std::string &table_name, Ts &&... args) {
  using namespace _impl;

  std::vector<std::string> names;
  (names.push_back(std::get<0>(args)), ...);

  std::vector<std::string> vals;
  (vals.push_back(_impl::sql_val_str(std::get<1>(args))), ...);

  std::string names_str = _impl::comma_delimit(std::begin(names), std::end(names));
  std::string vals_str = _impl::comma_delimit(std::begin(vals), std::end(vals));

  std::ostringstream oss;
  oss << "INSERT INTO " << table_name << " (" << names_str << ") VALUES (" << vals_str << ")";

  return oss.str();
}

/**
 * Builds a sqlite3 insert statement one column at a time
 */
struct InsertStatementBuilder {
  /**
   * Create a builder for a table with the given name
   */
  explicit InsertStatementBuilder(std::string table_name);

  /**
   * Add a column and value pair
   */
  void add(const std::string &col_name, int val);

  /**
   * Add a column and value pair
   */
  void add(const std::string &col_name, const std::string &val);

  /**
   * Output sql statement string
   */
  std::string string();

private:
  std::string table_name_;
  // Range of column, raw string value pairs
  std::vector<std::tuple<std::string, std::string>> col_vals_;
};

} // namespace sqlutil

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP
