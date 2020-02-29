//
// Created by Victor on 2/23/20.
//

#include "store.hpp"

#include <iostream>

#include <sqlite3.h>

using std::begin, std::end;

namespace {

int null_callback(void *,
                  int,
                  char **,
                  char **) {
  return 0;
}

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

CREATE TABLE IF NOT EXISTS runs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  algo TEXT NOT NULL,
  k INTEGER NOT NULL,
  hypergraph_id TEXT NOT NULL,
  cut_id INTEGER NOT NULL,
  time_elapsed_ms INTEGER NOT NULL,
  machine TEXT NOT NULL,
  commit_hash TEXT,
  time_taken INT NOT NULL,
  FOREIGN KEY (hypergraph_id)
    REFERENCES hypergraphs (id),
  FOREIGN KEY (cut_id)
    REFERENCES cuts (id)
);
)";

  char *zErrMsg{};
  err = sqlite3_exec(db_, kInitialize, null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  return true;
}

ReportStatus SqliteStore::report(const HypergraphWrapper &hypergraph) {
  size_t num_vertices = std::visit([](auto &&h) { return h.num_vertices(); }, hypergraph.h);
  size_t num_hyperedges = std::visit([](auto &&h) { return h.num_edges(); }, hypergraph.h);
  size_t size = std::visit([](auto &&h) { return h.size(); }, hypergraph.h);
  std::stringstream blob;
  std::visit([&blob](auto &&h) { blob << h; }, hypergraph.h);

  // Make statement
  std::stringstream stream;
  stream << "INSERT INTO hypergraphs (id, num_vertices, num_hyperedges, size, blob) VALUES ("
         << "'" << hypergraph.name << "'"
         << ", " << num_vertices
         << ", " << num_hyperedges
         << ", " << size
         << ", " << "'" << blob.str() << "'" << ");";

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, nullptr, &zErrMsg);
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
  std::stringstream stream;

  auto partition_to_str = [](const std::vector<int> &v) -> std::string {
    std::stringstream s;
    for (auto e : v) {
      s << e << " ";
    }
    std::string ret = s.str();

    return ret.substr(0, ret.size() - 1);
  };

  std::string table_name = "cuts"s + std::to_string(info.k);
  std::string columns = "(hypergraph_id, val, planted";
  for (int i = 0; i < info.k; ++i) {
    columns += ", size_p"s + std::to_string(i + 1);
  }
  for (int i = 0; i < info.k; ++i) {
    columns += ", blob_p"s + std::to_string(i + 1);
  }
  columns += ")";
  std::string partition_sizes_string;
  for (const auto &p : info.partitions) {
    partition_sizes_string += ", "s + std::to_string(p.size());
  }
  std::string partition_blobs_string;
  for (const auto &p : info.partitions) {
    partition_blobs_string += ", "s + "\'" + partition_to_str(p) + "\'";
  }
  stream << "INSERT INTO " << table_name << " " << columns << " VALUES ("
         << "'" << hypergraph_id << "'"
         << ", " << info.cut_value
         << ", " << (planted ? 1 : 0)
         << partition_sizes_string
         << partition_blobs_string
         << ");";

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return {ReportStatus::ERROR, {}};
  }
  sqlite3_int64 rowid = sqlite3_last_insert_rowid(db_);

  return {ReportStatus::OK, {static_cast<unsigned long long>(rowid)}};
}

ReportStatus SqliteStore::report(const std::string &hypergraph_id, const uint64_t cut_id, const CutRunInfo &info) {
  std::stringstream stream;

  // TODO git hash
  stream
      << "INSERT INTO runs (algo, k, hypergraph_id, cut_id, time_elapsed_ms, machine, time_taken) VALUES ("
      << "'" << info.algorithm << "'"
      << ", " << info.info.k
      << ", " << "'" << hypergraph_id << "'"
      << ", " << cut_id
      << ", " << info.time
      << ", " << "'" << info.machine << "'"
      << ", time(\"now\"))";

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, nullptr, &zErrMsg);
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


  // TODO make function
  auto partition_to_str = [](const std::vector<int> &v) -> std::string {
    std::stringstream s;
    for (auto e : v) {
      s << e << " ";
    }
    std::string ret = s.str();

    return ret.substr(0, ret.size() - 1);
  };

  std::stringstream query;
  query << "SELECT id FROM " << table_name << " WHERE hypergraph_id = '" << hypergraph_id << "'";
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
