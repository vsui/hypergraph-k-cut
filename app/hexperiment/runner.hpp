//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "common.hpp"

namespace hypergraph_util {
class ContractionStats;
}

class CutInfoStore;
class HypergraphSource;
class HypergraphGenerator;

using HypergraphCutFunc = std::function<HypergraphCut<size_t>(Hypergraph *,
                                                              uint64_t,
                                                              hypergraph_util::ContractionStats &)>;

using HypergraphCutValFunc = std::function<size_t(Hypergraph *, uint64_t, hypergraph_util::ContractionStats &)>;

class ExperimentRunner {
public:
  ExperimentRunner(const std::string &id,
                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                   std::shared_ptr<CutInfoStore> store,
                   bool planted,
                   bool cutoff,
                   size_t num_runs = 20);
  void run();

  void set_cutoff_percentages(const std::vector<size_t> &cutoffs);

private:

  template<bool ReturnsPartitions>
  struct CutFunc;

  template<>
  struct CutFunc<true> {
    using T = HypergraphCutFunc;
  };

  template<>
  struct CutFunc<false> {
    using T = HypergraphCutValFunc;
  };

  struct InitializeRet {
    size_t k;
    size_t cut_value;
    uint64_t planted_cut_id;
    HypergraphWrapper hypergraph;
    CutInfo planted_cut;
  };

  // Initializes the hypergraph by adding it to the database. Also adds the cut.
  // Returns an empty optional if an operation fails.
  std::optional<InitializeRet> doInitialize(const HypergraphGenerator &gen);

  // Add relevant data point to database
  template<bool ReturnsPartitions, bool CutOff = false>
  void doRun(const HypergraphGenerator &gen,
             const std::string &func_name,
             typename CutFunc<ReturnsPartitions>::T func,
             const CutInfo &planted_cut,
             const uint64_t planted_cut_id,
             size_t k);

  // Report cut to database, return cut ID or empty on failure
  template<bool ReturnsPartitions>
  std::optional<uint64_t> doReportCut(const HypergraphWrapper &hypergraph,
                                      const CutInfo &found_cut,
                                      const CutInfo &planted_cut,
                                      uint64_t planted_cut_id,
                                      bool cut_off);

  std::vector<std::pair<std::string, HypergraphCutFunc>> getCutAlgos(size_t k, size_t cut_value);

  std::vector<std::pair<std::string, HypergraphCutValFunc>> getCutValAlgos(const HypergraphWrapper &hypergraph,
                                                                           size_t k,
                                                                           size_t cut_value);

  template<typename T>
  bool notInFuncNames(T &&f);

  // The functions to use. If empty then use all functions
  std::vector<std::string> funcnames_;

  std::string id_;
  std::vector<std::unique_ptr<HypergraphGenerator>> src_;
  std::shared_ptr<CutInfoStore> store_;
  size_t num_runs_;
  bool planted_;
  bool cutoff_;
  std::vector<size_t> cutoff_percentages_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
