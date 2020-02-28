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

class CutInfoStore {
public:
  virtual ReportStatus report(const HypergraphWrapper &hypergraph) = 0;
  virtual ReportStatus report(const CutInfo &info, uint64_t &id) = 0;
  virtual ReportStatus report(const CutRunInfo &info) = 0;
  virtual ~CutInfoStore() = default;
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
