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

class FilesystemStore : public CutInfoStore {
public:
  FilesystemStore(std::filesystem::path path);

  bool initialize();

  ReportStatus report(const HypergraphWrapper &hypergraph) override;
  ReportStatus report(const CutInfo &info, uint64_t &id) override;
  ReportStatus report(const CutRunInfo &info) override;

private:
  std::filesystem::path root_path_;
  std::filesystem::path cuts_path_;
  std::filesystem::path runs_path_;
  std::filesystem::path hgrs_path_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_STORE_HPP
