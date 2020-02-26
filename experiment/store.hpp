//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP

#include <filesystem>

#include "common.hpp"

enum class ReportStatus {
  ERROR,
  OK,
  ALREADY_THERE,
};

class CutIterator {
public:
  CutIterator(std::filesystem::path path);

  CutInfo operator*();

  CutIterator &operator++();

  CutIterator begin();

  CutIterator end();

  bool operator!=(const CutIterator &it);

private:
  CutIterator();
  CutIterator(std::filesystem::directory_iterator hgrs, std::filesystem::directory_iterator cuts);

  bool end_ = false;
  std::filesystem::directory_iterator hgrs_;
  std::filesystem::directory_iterator cuts_;
};

class CutInfoStore {
public:
  virtual ReportStatus report(const HypergraphWrapper &hypergraph) = 0;
  virtual ReportStatus report(const CutInfo &info, uint64_t &id) = 0;
  virtual ReportStatus report(const CutRunInfo &info) = 0;
  virtual ~CutInfoStore() = default;
};

/**
 * Persists cut information to the filesystem.
 *
 * This stores all data in a directory at `path_`.
 *
 * The directory has this structure:
 * ```
 * root/
 * - cuts/
 *   - <name>/
 *     - <ID>.2cut
 *   - <name2>/
 *     - <ID>.3cut
 * - runs/
 *   - name.2cut.runs
 * - hypergraphs/
 *   - name.hgr
 * ```
 *
 * A 'hypergraphs/<name>.hgr' file is just a hypergraph in .hMETIS format.
 *
 * A 'cuts/<name>/<ID>.<k>cut' file holds a k-cut for the <name> hypergraph. A hypergraph may have multiple
 * cuts, hence the need for a cut ID.
 *
 * A runs/<name>.2cut.runs holds information about individual runs of a cut algorithm in a CSV format.
 */
class FilesystemStore : public CutInfoStore {
public:
  FilesystemStore(std::filesystem::path path);

  bool initialize();

  ReportStatus report(const HypergraphWrapper &hypergraph) override;
  ReportStatus report(const CutInfo &info, uint64_t &id) override;
  ReportStatus report(const CutRunInfo &info) override;

  CutIterator cuts();

private:
  std::filesystem::path root_path_;
  std::filesystem::path cuts_path_;
  std::filesystem::path runs_path_;
  std::filesystem::path hgrs_path_;
};

// Forward declaration for SqliteStore
class sqlite3;

/**
 * Persists experimental data to a sqlite database.
 */
class SqliteStore : public CutInfoStore {
public:
  bool open(std::filesystem::path db_path);

  ReportStatus report(const HypergraphWrapper &hypergraph) override;
  ReportStatus report(const CutInfo &info, uint64_t &id) override;
  ReportStatus report(const CutRunInfo &info) override;

  ~SqliteStore();
private:
  sqlite3 *db_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP
