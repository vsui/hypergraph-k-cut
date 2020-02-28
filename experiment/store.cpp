//
// Created by Victor on 2/23/20.
//

#include "store.hpp"

#include <iostream>

#include <sqlite3.h>

using std::begin, std::end;

namespace {

static int null_callback([[maybe_unused]] void *not_used,
                         [[maybe_unused]] int argc,
                         [[maybe_unused]] char **argv,
                         [[maybe_unused]] char **col_names) {
  return 0;
}

}

bool SqliteStore::open(std::filesystem::path db_path) {
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
  err = sqlite3_exec(db_, kInitialize, null_callback, 0, &zErrMsg);
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
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, 0, &zErrMsg);
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

ReportStatus SqliteStore::report(const CutInfo &info, [[maybe_unused]] uint64_t &id) {
  std::stringstream stream;
  size_t planted = 0; // TODO

  using namespace std::string_literals;

  // Make sure partitions are sorted
  std::vector<std::vector<int>> partitions = info.partitions;
  for (auto &partition : partitions) {
    std::sort(begin(partition), end(partition));
  }
  std::sort(begin(partitions), end(partitions), [](std::vector<int> &a, std::vector<int> &b) {
    if (a.size() == b.size()) {
      return a < b;
    }
    return a.size() < b.size();
  });

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
  for (const auto &p : partitions) {
    partition_sizes_string += ", "s + std::to_string(p.size());
  }
  std::string partition_blobs_string;
  for (const auto &p : partitions) {
    partition_blobs_string += ", "s + "\'" + partition_to_str(p) + "\'";
  }
  stream << "INSERT INTO " << table_name << " " << columns << " VALUES ("
         << "'" << info.hypergraph << "'"
         << ", " << info.cut_value
         << ", " << 0 /* NOT PLANTED */
         << partition_sizes_string
         << partition_blobs_string
         << ");";

  char *zErrMsg{};
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, 0, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return ReportStatus::ERROR;
  }
  sqlite3_int64 rowid = sqlite3_last_insert_rowid(db_);
  id = rowid;

  return ReportStatus::OK;
}

ReportStatus SqliteStore::report(const CutRunInfo &info) {
  std::stringstream stream;
  std::string hypergraph_id = info.info.hypergraph;
  size_t cut_id = info.info.id;

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
  int err = sqlite3_exec(db_, stream.str().c_str(), null_callback, 0, &zErrMsg);
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
