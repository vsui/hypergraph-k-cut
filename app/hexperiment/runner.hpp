//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <optional>
#include <hypergraph/cxy.hpp>
#include <hypergraph/fpz.hpp>
#include <hypergraph/kk.hpp>
#include "ThreadPool/ThreadPool.h"

#include "common.hpp"

// TODO don't forward declare things from external libraries
namespace hypergraphlib::util {
class ContractionStats;
}

class CutInfoStore;
class HypergraphGenerator;

using HypergraphCutFunc = std::function<hypergraphlib::HypergraphCut<size_t>(hypergraphlib::Hypergraph *,
                                                                             uint64_t,
                                                                             hypergraphlib::util::ContractionStats &)>;

using HypergraphCutValFunc = std::function<size_t(hypergraphlib::Hypergraph *,
                                                  uint64_t,
                                                  hypergraphlib::util::ContractionStats &)>;

class ExperimentRunner {
public:
  ExperimentRunner(std::string id,
                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                   std::shared_ptr<CutInfoStore> store,
                   bool planted,
                   size_t num_runs);
  virtual void run();

  virtual ~ExperimentRunner() = default;

protected:
  template<bool ReturnsPartitions>
  using CutFunc = std::conditional_t<ReturnsPartitions, HypergraphCutFunc, HypergraphCutValFunc>;

  size_t num_runs() {
    return num_runs_;
  }

  const std::string &id() {
    return id_;
  }

  CutInfoStore &store() {
    return *store_;
  }

protected:
  std::mutex mut_here;

  // Report cut to database, return cut ID or empty on failure
  template<bool ReturnsPartitions>
  std::optional<uint64_t> doReportCut(const HypergraphWrapper &hypergraph,
                                      const CutInfo &found_cut,
                                      const CutInfo &planted_cut,
                                      uint64_t planted_cut_id,
                                      bool cut_off = false);

  // Report cut and run to database
  template<bool ReturnsPartitions>
  bool doReportCutAndRun(const HypergraphWrapper &hypergraph,
                         const CutInfo &found_cut_info,
                         const CutInfo &planted_cut,
                         uint64_t planted_cut_id,
                         const CutRunInfo &run_info,
                         const hypergraphlib::util::ContractionStats &stats);

private:

  struct InitializeRet {
    size_t k = 0;
    size_t cut_value = 0;
    uint64_t planted_cut_id = 0;
    HypergraphWrapper hypergraph;
    CutInfo planted_cut;
  };

  // Initializes the hypergraph by adding it to the database. Also adds the cut.
  // Returns an empty optional if an operation fails.
  std::optional<InitializeRet> doInitialize(const HypergraphGenerator &gen);

  // Do work on hypergraph
  virtual void doProcessHypergraph(const HypergraphGenerator &gen,
                                   const HypergraphWrapper &hypergraph,
                                   size_t k,
                                   size_t cut_value,
                                   const CutInfo &planted_cut,
                                   size_t planted_cut_id) = 0;

  std::string id_;
  std::vector<std::unique_ptr<HypergraphGenerator>> src_;
  std::shared_ptr<CutInfoStore> store_;
  size_t num_runs_;
  bool planted_;

};

class DiscoveryRunner : public ExperimentRunner {
  // Add relevant data point to database
public:
  DiscoveryRunner(std::string id,
                  std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                  std::shared_ptr<CutInfoStore> store,
                  bool planted,
                  size_t num_runs,
                  std::vector<std::string> func_names);

  void doProcessHypergraph(const HypergraphGenerator &gen,
                           const HypergraphWrapper &hypergraph,
                           size_t k,
                           size_t cut_value,
                           const CutInfo &planted_cut,
                           size_t planted_cut_id) override;

  virtual void run() override;
private:
  std::vector<std::future<void>> futures_;

  // The functions to use. If empty then use all functions
  std::vector<std::string> funcnames_;

  ThreadPool pool;
  std::mutex mut;

  template<bool ReturnsPartitions>
  void doRunDiscovery(const HypergraphGenerator &gen,
                      const std::string &func_name,
                      CutFunc <ReturnsPartitions> func,
                      const CutInfo &planted_cut,
                      uint64_t planted_cut_id,
                      size_t k);

  template<typename T>
  bool notInFuncNames(T &&f);

  std::vector<std::pair<std::string, HypergraphCutFunc>> getCutAlgos(size_t k, size_t cut_value);

  std::vector<std::pair<std::string, HypergraphCutValFunc>> getCutValAlgos(const HypergraphWrapper &hypergraph,
                                                                           size_t k,
                                                                           size_t cut_value);

};

class CutoffRunner : public ExperimentRunner {
public:
  CutoffRunner(std::string id,
               std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
               std::shared_ptr<CutInfoStore>
               store,
               bool planted,
               size_t num_runs,
               std::vector<std::string> algos,
               std::vector<double> cutoff_percentages,
               std::filesystem::path output_dir);

  void doProcessHypergraph(const HypergraphGenerator &gen,
                           const HypergraphWrapper &hypergraph,
                           size_t k,
                           size_t cut_value,
                           const CutInfo &planted_cut,
                           size_t planted_cut_id) override;

  // Store for all the runs. We need a special store since cutoff runner does not write to
  // the sqlite database. This makes it easier to collect all the data in different threads
  class RunStore {
  public:
    std::map<double, std::vector<double>> kk_runs() { return kk_runs_; }
    std::map<double, std::vector<double>> fpz_runs() { return fpz_runs_; }
    std::map<double, std::vector<double>> cxy_runs() { return cxy_runs_; }

    void write_to_kk(double percentage, const double factor) {
      std::lock_guard lock(mut);
      kk_runs_[percentage].emplace_back(factor);
    }
    void write_to_fpz(double percentage, const double factor) {
      std::lock_guard lock(mut);
      fpz_runs_[percentage].emplace_back(factor);
    }
    void write_to_cxy(double percentage, const double factor) {
      std::lock_guard lock(mut);
      cxy_runs_[percentage].emplace_back(factor);
    }

    template<typename ContractImpl>
    void write_to(const double percentage, const double factor) {
      if constexpr (std::is_same_v<ContractImpl, hypergraphlib::cxy>) {
        write_to_cxy(percentage, factor);
      } else if constexpr (std::is_same_v<ContractImpl, hypergraphlib::fpz>) {
        write_to_fpz(percentage, factor);
      } else if constexpr (std::is_same_v<ContractImpl, hypergraphlib::kk>) {
        write_to_kk(percentage, factor);
      } else {
        std::cerr << "Bad contraction impl" << std::endl;
        exit(1);
      }
    }

  private:
    // Write mutex
    std::mutex mut;

    // Each of these maps is a map from the percentage to the cut factors
    std::map<double, std::vector<double>> kk_runs_;
    std::map<double, std::vector<double>> fpz_runs_;
    std::map<double, std::vector<double>> cxy_runs_;
  };

private:
  RunStore cutoff_store;
  ThreadPool pool;

  std::chrono::duration<double> computeCutoffTime(const HypergraphWrapper &hypergraph);

  template<typename ContractImpl>
  void doRunCutoff(const HypergraphWrapper &hypergraph,
                   size_t k,
                   size_t discovery_value,
                   std::chrono::duration<double> cutoff_time);

  template<bool ReturnsPartitions>
  void doRunDiscovery(const HypergraphGenerator &gen,
                      const std::string &func_name,
                      CutFunc <ReturnsPartitions> func,
                      const CutInfo &planted_cut,
                      uint64_t planted_cut_id,
                      size_t k);

  std::vector<double> cutoff_percentages_;

  std::filesystem::path output_dir_;

  std::vector<std::string> algos_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_EVALUATOR_HPP
