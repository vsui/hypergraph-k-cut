//
// Created by Victor on 2/23/20.
//

#include <iostream>

#include <sqlite3.h>
#include <generators/generators.hpp>

#include "store.hpp"
#include "sqlutil.hpp"

using std::begin, std::end;

namespace {

int null_callback(void *,
                  int,
                  char **,
                  char **) {
  return 0;
}

std::string partition_to_str(const std::vector<int> &v) {
  std::stringstream s;
  for (auto e : v) {
    s << e << " ";
  }
  std::string ret = s.str();

  return ret.substr(0, ret.size() - 1);
}

// *M*ake string-*I*nt *T*uple
constexpr auto mit = std::make_tuple<std::string, int>;
// *M*ake string-*S*tring *T*uple
std::tuple<std::string, std::string> mst(const std::string &name, const std::string &val) {
  return std::make_tuple(std::string(name), std::string(val));
}

}

ReportStatus SqliteStore::report(const HypergraphGenerator &h) {
  if (!h.write_to_table(db_)) {
    std::cout << "Failed to write hypergraph generator info to DB" << std::endl;
    return ReportStatus::ERROR;
  }

  const auto[hypergraph, _] = h.generate();
  HypergraphWrapper w;
  w.h = hypergraph;
  w.name = h.name();

  return report(w);
}

bool SqliteStore::open(const std::filesystem::path &db_path) {
  if (db_ != nullptr) {
    std::cerr << "Database already open" << std::endl;
    return false;
  }

  int err = sqlite3_open(db_path.c_str(), &db_);

  if (err) {
    std::cerr << "Cannot open database: " << db_path << std::endl;
    return false;
  }

  static constexpr char kInitialize[] = R"(
CREATE TABLE IF NOT EXISTS hypergraphs (
  id TEXT PRIMARY KEY,
  num_vertices INTEGER NOT NULL,
  num_hyperedges INTEGER NOT NULL,
  size INTEGER NOT NULL,
  blob BLOB NOT NULL
);

CREATE TABLE IF NOT EXISTS ring_hypergraphs (
  id INTEGER PRIMARY KEY,
  FOREIGN KEY(id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS cuts2 (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  size_p1 INTEGER,
  size_p2 INTEGER,
  blob_p1 BLOB,
  blob_p2 BLOB,
  FOREIGN KEY (hypergraph_id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS cuts3 (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  size_p1 INTEGER,
  size_p2 INTEGER,
  size_p3 INTEGER,
  blob_p1 BLOB,
  blob_p2 BLOB,
  blob_p3 BLOB,
  FOREIGN KEY (hypergraph_id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS cuts4 (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  size_p1 INTEGER,
  size_p2 INTEGER,
  size_p3 INTEGER,
  size_p4 INTEGER,
  blob_p1 BLOB,
  blob_p2 BLOB,
  blob_p3 BLOB,
  blob_p4 BLOB,
  FOREIGN KEY (hypergraph_id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS cuts5 (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  size_p1 INTEGER,
  size_p2 INTEGER,
  size_p3 INTEGER,
  size_p4 INTEGER,
  size_p5 INTEGER,
  blob_p1 BLOB,
  blob_p2 BLOB,
  blob_p3 BLOB,
  blob_p4 BLOB,
  blob_p5 BLOB,
  FOREIGN KEY (hypergraph_id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS cuts6 (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  size_p1 INTEGER,
  size_p2 INTEGER,
  size_p3 INTEGER,
  size_p4 INTEGER,
  size_p5 INTEGER,
  size_p6 INTEGER,
  blob_p1 BLOB,
  blob_p2 BLOB,
  blob_p3 BLOB,
  blob_p4 BLOB,
  blob_p5 BLOB,
  blob_p6 BLOB,
  FOREIGN KEY (hypergraph_id)
  REFERENCES hypergraphs (id)
);

CREATE TABLE IF NOT EXISTS runs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  algo TEXT NOT NULL,
  k INTEGER NOT NULL,
  hypergraph_id TEXT NOT NULL,
  cut_id INTEGER,
  time_elapsed_ms INTEGER NOT NULL,
  machine TEXT NOT NULL,
  commit_hash TEXT,
  time_taken INT NOT NULL,
  num_runs_for_discovery INT,
  num_contractions INT,
  experiment_id TEXT,
  FOREIGN KEY (hypergraph_id)
    REFERENCES hypergraphs (id),
  FOREIGN KEY (cut_id)
    REFERENCES cuts (id)
);
)";

  std::string sql_command = std::string(kInitialize) + PlantedHypergraph::make_table_sql_command();
  char *zErrMsg{};
  err = sqlite3_exec(db_, sql_command.c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return false;
  }

  std::cout << "Opened database at " << db_path << std::endl;

  return true;
}

ReportStatus SqliteStore::report(const HypergraphWrapper &hypergraph) {
  size_t num_vertices = std::visit([](auto &&h) { return h.num_vertices(); }, hypergraph.h);
  size_t num_hyperedges = std::visit([](auto &&h) { return h.num_edges(); }, hypergraph.h);
  size_t size = std::visit([](auto &&h) { return h.size(); }, hypergraph.h);
  std::stringstream blob;
  std::visit([&blob](auto &&h) { blob << h; }, hypergraph.h);

  // Make statement
  const std::string stmt = sqlutil::insert_statement(
      "hypergraphs",
      mst("id", hypergraph.name),
      mit("num_vertices", num_vertices),
      mit("num_hyperedges", num_hyperedges),
      mit("size", size),
      mst("blob", blob.str())
  );

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stmt.c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    // May not be the best way to do this..
    if (std::string(zErrMsg) == "UNIQUE constraint failed: hypergraphs.id") {
      return ReportStatus::ALREADY_THERE;
    }
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return ReportStatus::ERROR;
  }

  return ReportStatus::OK;
}

std::tuple<ReportStatus, uint64_t> SqliteStore::report(const std::string &hypergraph_id,
                                                       const CutInfo &info,
                                                       bool planted) {
  using namespace std::string_literals;

  // First check if the cut is already present
  std::optional<std::tuple<bool, uint64_t>> cut_already_present = has_cut(hypergraph_id, info);

  if (!cut_already_present.has_value()) {
    // Failed to query value
    std::cout << "Failed to check if cut was already present in database" << std::endl;
    return {ReportStatus::ERROR, {}};
  }

  const auto[already_present, cut_id] = cut_already_present.value();
  if (already_present) {
    return {ReportStatus::ALREADY_THERE, cut_id};
  }

  // Cut not in store, need to add it
  sqlutil::InsertStatementBuilder builder("cuts"s + std::to_string(info.k));
  builder.add("hypergraph_id", hypergraph_id);
  builder.add("val", info.cut_value);
  builder.add("planted", planted ? 1 : 0);
  for (int i = 0; i < info.partitions.size(); ++i) {
    builder.add("size_p"s + std::to_string(i + 1), info.partitions.at(i).size());
    builder.add("blob_p"s + std::to_string(i + 1), partition_to_str(info.partitions.at(i)));
  }
  const auto insert_stmt = builder.string();

  char *zErrMsg{};
  int err = sqlite3_exec(db_, insert_stmt.c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return {ReportStatus::ERROR, {}};
  }
  sqlite3_int64 rowid = sqlite3_last_insert_rowid(db_);

  return {ReportStatus::OK, {static_cast<unsigned long long>(rowid)}};
}

ReportStatus SqliteStore::report(const std::string &hypergraph_id,
                                 const uint64_t cut_id,
                                 const CutRunInfo &info,
                                 const size_t num_runs_for_discovery,
                                 const size_t num_contractions) {
  // TODO git hash
  const std::string stmt = sqlutil::insert_statement(
      "runs",
      mst("algo", info.algorithm),
      mit("k", info.info.k),
      mst("hypergraph_id", hypergraph_id),
      mit("cut_id", cut_id),
      mit("time_elapsed_ms", info.time),
      mst("machine", info.machine),
      std::make_tuple<std::string, sqlutil::TimeNow>("time_taken", {}),
      mst("experiment_id", info.experiment_id),
      mit("num_runs_for_discovery", num_runs_for_discovery),
      mit("num_contractions", num_contractions)
  );

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stmt.c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return ReportStatus::ERROR;
  }

  return ReportStatus::OK;
}

ReportStatus SqliteStore::report(const std::string &hypergraph_id,
                                 const CutRunInfo &info,
                                 size_t num_runs_for_discovery,
                                 size_t num_contractions) {
  const std::string stmt = sqlutil::insert_statement(
      "runs",
      mst("algo", info.algorithm),
      mit("k", info.info.k),
      mst("hypergraph_id", hypergraph_id),
      mit("time_elapsed_ms", info.time),
      mst("machine", info.machine),
      std::make_tuple<std::string, sqlutil::TimeNow>("time_taken", {}),
      mst("experiment_id", info.experiment_id),
      mit("num_runs_for_discovery", num_runs_for_discovery),
      mit("num_contractions", num_contractions)
  );

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stmt.c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return ReportStatus::ERROR;
  }
  return ReportStatus::OK;
}

SqliteStore::~SqliteStore() {
  sqlite3_close(db_);
}

// Returns (has info, ID if has info). Optional is empty on failure
std::optional<std::tuple<bool, uint64_t>> SqliteStore::has_cut(const std::string &hypergraph_id, const CutInfo &info) {
  using namespace std::string_literals;

  std::string table_name = "cuts"s + std::to_string(info.k);

  std::stringstream query;
  query << "SELECT id FROM " << table_name << " WHERE hypergraph_id = '" << hypergraph_id << "'" << " AND val = "
        << info.cut_value;
  for (int i = 0; i < info.partitions.size(); ++i) {
    const auto &partition = info.partitions[i];
    query << " AND " << "size_p" << std::to_string(i + 1) << " = " << partition.size()
          << " AND " << "blob_p" << std::to_string(i + 1) << " = '" << partition_to_str(partition) << "'";
  }
  query << " LIMIT 1;";

  std::tuple<bool, uint64_t> result;

  char *zErrMsg{};
  int err = sqlite3_exec(db_, query.str().c_str(), [](void *input, int argc, char **argv, char **col_names) {
    using RetT = std::tuple<bool, uint64_t>;
    RetT &result = *reinterpret_cast<RetT *>(input);
    if (argc == 0) {
      result = {false, 0};
    }

    // Otherwise parse ID
    std::stringstream ss;
    ss << argv[0];
    uint64_t id;
    ss >> id;

    result = {true, id};
    return 0;
  }, &result, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return {};
  }

  return {result};
}
