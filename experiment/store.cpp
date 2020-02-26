//
// Created by Victor on 2/23/20.
//

#include "store.hpp"

#include <iostream>
#include <fstream>

#include <sqlite3.h>

FilesystemStore::FilesystemStore(std::filesystem::path path)
    : root_path_(path), cuts_path_(path / "cuts"), runs_path_(path / "runs"), hgrs_path_(path / "hypergraphs") {}

namespace fs = std::filesystem;
using std::begin, std::end;

CutIterator::CutIterator(fs::path path) : hgrs_(path), cuts_(hgrs_->path()), end_(false) {}

CutInfo CutIterator::operator*() {
  CutInfo info;

  std::ifstream in(cuts_->path(), std::ios_base::in);
  in >> info;
  info.id = std::stoul(cuts_->path().filename());
  return info;
}

CutIterator &CutIterator::operator++() {
  ++cuts_;
  if (cuts_ == fs::end(cuts_)) {
    ++hgrs_;
    if (hgrs_ == fs::end(hgrs_)) {
      end_ = true;
    } else {
      cuts_ = fs::directory_iterator(hgrs_->path());
    }
  }
  return *this;
}

CutIterator::CutIterator() : end_(true) {}

CutIterator CutIterator::begin() { return *this; }

CutIterator CutIterator::end() { return CutIterator(); }

bool CutIterator::operator!=(const CutIterator &it) {
  if (end_) {
    return !it.end_;
  }
  return hgrs_ != it.hgrs_ || cuts_ != it.cuts_;
}

namespace {

/**
 * Return true if path is a directory, attempts to create such a directory otherwise and returns true on success
 * @param path
 * @return
 */
bool probe(const fs::path &path) {
  if (!fs::exists(path)) {
    return fs::create_directory(path);
  }
  return fs::is_directory(path);
}

}

bool FilesystemStore::initialize() {
  return probe(root_path_) && probe(cuts_path_) && probe(runs_path_) && probe(hgrs_path_);
}

ReportStatus FilesystemStore::report(const HypergraphWrapper &hypergraph) {
  // Check if already present
  auto dir_it = fs::directory_iterator(hgrs_path_);
  auto it = std::find_if(begin(dir_it),
                         end(dir_it),
                         [&hypergraph](const fs::directory_entry &entry) {
                           return entry.path().filename().string() == hypergraph.name;
                         });
  if (it != end(dir_it)) {
    return ReportStatus::ALREADY_THERE;
  }

  // Need to create it
  fs::path path(hgrs_path_ / hypergraph.name);
  std::ofstream out(path);

  struct {
    std::ofstream *out;
    void operator()(const Hypergraph &h) { *out << h; }
    void operator()(const WeightedHypergraph<size_t> &h) { *out << h; }
  } visitor;
  visitor.out = &out;

  std::visit(visitor, hypergraph.h);

  return ReportStatus::OK;
}

ReportStatus FilesystemStore::report(const CutInfo &info, uint64_t &id) {
  // Check if already present
  fs::path cuts_dir = cuts_path_ / info.hypergraph;

  if (!probe(cuts_dir)) {
    return ReportStatus::ERROR;
  }

  // Need to check each file for if there is a cut inside it
  auto dir_it = fs::directory_iterator(cuts_dir);

  // See if there is a file where they are equal
  auto it = std::find_if(begin(dir_it), end(dir_it), [info](const fs::directory_entry &entry) {
    std::ifstream in(entry.path());
    CutInfo in_info;
    in >> in_info;
    return in_info == info;
  });

  if (it != end(dir_it)) {
    return ReportStatus::ALREADY_THERE;
  }

  size_t distance = std::distance(begin(dir_it), end(dir_it));
  id = distance;
  std::ofstream out(cuts_dir / std::to_string(distance));
  out << info;

  return ReportStatus::OK;
}

ReportStatus FilesystemStore::report(const CutRunInfo &info) {
  fs::path path = runs_path_ / info.info.hypergraph;

  bool existed = fs::exists(path);

  std::ofstream out(runs_path_ / info.info.hypergraph, std::ios_base::app);

  if (!existed) {
    out << CutRunInfo::csv_header() << std::endl;
  }

  out << info << std::endl;

  return ReportStatus::OK;
}

CutIterator FilesystemStore::cuts() {
  return CutIterator(cuts_path_);
}

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

CREATE TABLE IF NOT EXISTS cuts (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  hypergraph_id TEXT,
  k INTEGER NOT NULL,
  val INTEGER NOT NULL,
  planted INTEGER NOT NULL,
  skewed INTEGER NOT NULL,
  partitions BLOB,
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
  std::stringstream blob;
  int skewed = std::any_of(begin(info.partitions), end(info.partitions), [](auto &&part) {
    return part.size() == 1;
  });

  blob << info;

  stream << "INSERT INTO cuts (hypergraph_id, k, val, planted, partitions, skewed) VALUES ("
         << "'" << info.hypergraph << "'"
         << ", " << info.k << ", "
         << info.cut_value
         << ", " << 1337
         << ", " << "'" << blob.str() << "'"
         << ", " << skewed << ");";

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
