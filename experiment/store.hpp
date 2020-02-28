//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP

#include <filesystem>
#include <tuple>

#include "common.hpp"

enum class ReportStatus {
  ERROR,
  OK,
  ALREADY_THERE,
};

class CutInfoStore {
public:
  /**
   * Report a hypergraph to the store. Does nothing if the hypergraph is already in the store.
   *
   * @param hypergraph
   * @return
   */
  virtual ReportStatus report(const HypergraphWrapper &hypergraph) = 0;

  /**
   * Report a cut to the store and return the store's internal ID of the cut. If the cut is not in the store then a new
   * ID will be created. If the cut is already in the store then its proper ID will be returned and no changes will be
   * made to the store. On error the returned ID has no meaning.
   *
   * @param hypergraph_id
   * @param info
   * @return
   */
  virtual std::tuple<ReportStatus, uint64_t> report(const std::string &hypergraph_id, const CutInfo &info) = 0;

  /**
   * Report a run to the store.
   *
   * @param info
   * @return
   */
  virtual ReportStatus report(const std::string &hypergraph_id, uint64_t cut_id, const CutRunInfo &info) = 0;

  virtual ~CutInfoStore() = default;
};


// Forward declaration for SqliteStore
class sqlite3;

/**
 * Persists experimental data to a sqlite database.
 */
class SqliteStore : public CutInfoStore {
public:
  bool open(const std::filesystem::path& db_path);

  ReportStatus report(const HypergraphWrapper &hypergraph) override;
  std::tuple<ReportStatus, uint64_t> report(const std::string &hypergraph_id, const CutInfo &info) override;
  ReportStatus report(const std::string &hypergraph_id, uint64_t cut_id, const CutRunInfo &info) override;

  ~SqliteStore();
private:
  sqlite3 *db_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP
