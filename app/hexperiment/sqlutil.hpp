//
// Created by Victor on 3/7/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP

namespace sqlutil {

/* Struct representing `time("now")` SQL function */
struct TimeNow {};

namespace _impl {
template<typename InputIt>
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

} // namespace sqlutil

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_SQLUTIL_HPP
