//
// Created by Victor on 2/23/20.
//

#include <unistd.h>

#include "runner.hpp"
#include "store.hpp"
#include "generators.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <hypergraph/order.hpp>
#include <hypergraph/hypergraph.hpp>
#include <hypergraph/cxy.hpp>
#include <hypergraph/fpz.hpp>
#include <hypergraph/kk.hpp>

namespace {

std::string hostname() {
  std::vector<char> hostname(50, '0');
  gethostname(hostname.data(), 49);
  return {hostname.data()};
}

}

ExperimentRunner::ExperimentRunner(std::string id,
                                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                   std::shared_ptr<CutInfoStore> store,
                                   bool planted,
                                   size_t num_runs) : id_(std::move(id)),
                                                      src_(std::move(source)),
                                                      store_(std::move(store)),
                                                      planted_(planted),
                                                      num_runs_(num_runs) {}

void ExperimentRunner::run() {
  spdlog::info("Beginning experiment");

  // Log the sizes of each hypergraph so we can decide whether or not we want to do this...
  for (const auto &gen : src_) {
    const auto[hgraph, planted_cut_optional] = gen->generate();
    spdlog::info("size of {}: {}", gen->name(), hgraph.size());
  }

  for (const auto &gen : src_) {
    const auto init = doInitialize(*gen);
    if (!init) {
      spdlog::error("Failed to initialize {}", gen->name());
      continue;
    }
    const auto &[k, cut_value, planted_cut_id, hypergraph, planted_cut] = init.value();

    spdlog::info("[{}] Collecting data for hypergraph", hypergraph.name);

    // This part is different
    doProcessHypergraph(*gen, hypergraph, k, cut_value, planted_cut, planted_cut_id);
  }
}

std::optional<ExperimentRunner::InitializeRet> ExperimentRunner::doInitialize(const HypergraphGenerator &gen) {
  const auto[hgraph, planted_cut_optional] = gen.generate();
  HypergraphWrapper hypergraph = {.h = hgraph, .name = gen.name()};

  ReportStatus status = store_->report(hypergraph);
  if (status == ReportStatus::ERROR) {
    spdlog::error("Failed to put hypergraph info in DB");
    return {};
  }

  if (!planted_) {
    spdlog::info("Cut not planted, computing min cut exactly...");
  }
  // Need to do this since planted_cut does not have a default constructor
  CutInfo planted_cut = planted_ ? planted_cut_optional.value() :
                        [&hypergraph]() {
                          Hypergraph *hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);
                          if (hypergraph_ptr == nullptr) {
                            throw std::runtime_error("Hypergraph is null");
                          }
                          // Copy to call MW_min_cut since it can write to the hypergraph
                          Hypergraph temp(*hypergraph_ptr);
                          return CutInfo{2, MW_min_cut(temp)};
                        }();

  if (!planted_) {
    // Skip if the cut is uninteresting or skewed
    if (planted_cut.cut_value == 0) {
      spdlog::warn("Skipping {}, hypergraph is disconnected", hypergraph.name);
      return {};
    }
    // We are assuming here that there are two partitions since as of now there is no way to get a cut efficiently
    // where k>2
    double_t skew = static_cast<double>(planted_cut.partitions[0].size()) / hgraph.num_vertices();
    if (skew < 0.1 || skew > 0.9) {
      spdlog::warn("Skipping {}, cut is skewed ({}, {})",
                   hypergraph.name,
                   planted_cut.partitions.at(0).size(),
                   planted_cut.partitions.at(1).size());
      return {};
    }
  }

  uint64_t planted_cut_id;
  std::tie(status, planted_cut_id) = store_->report(hypergraph.name, planted_cut, true);
  if (status == ReportStatus::ERROR) {
    spdlog::error("Failed to put planted cut info in DB");
    return {};
  }

  InitializeRet ret{
      .k = planted_cut.k,
      .cut_value = planted_cut.cut_value,
      .planted_cut_id = planted_cut_id,
      .hypergraph = hypergraph,
      .planted_cut = planted_cut
  };
  return {ret};
}

void ExperimentRunner::set_cutoff_percentages(const std::vector<size_t> &cutoffs) {
  cutoff_percentages_ = cutoffs;
}

template<bool ReturnsPartitions>
void DiscoveryRunner::doRunDiscovery(const HypergraphGenerator &gen,
                                     const std::string &func_name,
                                     typename CutFunc<ReturnsPartitions>::T func,
                                     const CutInfo &planted_cut,
                                     const uint64_t planted_cut_id,
                                     size_t k) {
  // Make sources of randomness
  std::random_device rd;
  std::mt19937_64 rgen(rd());
  std::uniform_int_distribution<uint64_t> dis;

  const auto[hgraph, planted_cut_optional] = gen.generate();
  HypergraphWrapper hypergraph;
  hypergraph.h = hgraph;
  hypergraph.name = gen.name();
  // TODO make this unnecessary
  // Avoid this variant foolishness for now
  Hypergraph *hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);
  assert(hypergraph_ptr != nullptr);

  spdlog::info("[{} / {}] Starting", hypergraph.name, func_name);
  for (int i = 0; i < num_runs(); ++i) {
    spdlog::info("[{} / {}] Run {}/{}", hypergraph.name, func_name, i + 1, num_runs());

    // We have to make a copy of the hypergraph since some algorithms write to it
    Hypergraph temp(*hypergraph_ptr);
    hypergraph_util::ContractionStats stats{};

    auto start = std::chrono::high_resolution_clock::now();
    auto cut = func(&temp, dis(rgen), stats);
    auto stop = std::chrono::high_resolution_clock::now();

    CutInfo found_cut_info(k, cut);

    CutRunInfo run_info(id(), found_cut_info);
    run_info.algorithm = func_name;
    run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    run_info.machine = hostname();
    run_info.commit = "n/a";

    auto found_cut_id =
        doReportCut<ReturnsPartitions>(hypergraph, found_cut_info, planted_cut, planted_cut_id);
    if (!found_cut_id) {
      spdlog::error("Failed to get cut ID");
      return;
    }

    if (store().report(hypergraph.name,
                       found_cut_id.value(),
                       run_info,
                       stats.num_runs,
                       stats.num_contractions)
        == ReportStatus::ERROR) {
      spdlog::error("Failed to report run");
    }
  }
}
void DiscoveryRunner::doProcessHypergraph(const HypergraphGenerator &gen,
                                          const HypergraphWrapper &hypergraph,
                                          const size_t k,
                                          const size_t cut_value,
                                          const CutInfo &planted_cut,
                                          const size_t planted_cut_id) {
  // This should be the new code...

  for (const auto &[func_name, func] : getCutAlgos(k, cut_value)) {
    doRunDiscovery<true>(gen, func_name, func, planted_cut, planted_cut_id, k);
  }
  for (const auto &[func_name, func] : getCutValAlgos(hypergraph, k, cut_value)) {
    doRunDiscovery<false>(gen, func_name, func, planted_cut, planted_cut_id, k);
  }
  // This part is different
}
DiscoveryRunner::DiscoveryRunner(std::string id,
                                 std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                 std::shared_ptr<CutInfoStore> store,
                                 bool planted,
                                 size_t num_runs,
                                 std::vector<std::string> func_names) : ExperimentRunner(std::move(id),
                                                                                         std::move(source),
                                                                                         std::move(store),
                                                                                         planted,
                                                                                         num_runs),
                                                                        funcnames_(std::move(func_names)) {}

template<>
std::optional<uint64_t> ExperimentRunner::doReportCut<true>(const HypergraphWrapper &hypergraph,
                                                            const CutInfo &found_cut,
                                                            const CutInfo &planted_cut,
                                                            uint64_t planted_cut_id,
                                                            bool /* cutoff */) {
  // Check if found cut was the planted cut
  uint64_t found_cut_id;
  if (found_cut == planted_cut) {
    found_cut_id = planted_cut_id;
  } else {
    // Otherwise we need to report this cut to the DB to get its ID
    if (found_cut.cut_value == planted_cut.cut_value) {
      spdlog::warn("Found cut has same value as planted cut ({}) but for different partition sizes");
    } else {
      spdlog::warn("Found cut value {} is not the planted cut value {}",
                   found_cut.cut_value,
                   planted_cut.cut_value);
    }
    ReportStatus status;
    std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut, false);
    if (status == ReportStatus::ERROR) {
      spdlog::error("Failed to report found cut");
      return {};
    }
  }
  return found_cut_id;
}

template<>
std::optional<uint64_t> ExperimentRunner::doReportCut<false>(const HypergraphWrapper &hypergraph,
                                                             const CutInfo &found_cut,
                                                             const CutInfo &planted_cut,
                                                             uint64_t,
                                                             bool cut_off) {
  // Check if found cut value was lower than the planted cut
  if (found_cut.cut_value < planted_cut.cut_value) {
    spdlog::warn("Found cut has lesser value than the planted cut");
  }

  if (found_cut.cut_value > planted_cut.cut_value) {
    if (cut_off) {
      spdlog::info("Cut off at {} {}", found_cut.cut_value, planted_cut.cut_value);
    } else {
      spdlog::error("Found cut has greater value than planted cut (should not be possible)");
    }
  }

  ReportStatus status;
  uint64_t found_cut_id;
  std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut, false);
  if (status == ReportStatus::ERROR) {
    spdlog::error("Failed to report found cut");
    return {};
  }

  return found_cut_id;
}

std::vector<std::pair<std::string, HypergraphCutFunc>> DiscoveryRunner::getCutAlgos(const size_t k,
                                                                                    const size_t cut_value) {
  // TODO I should be checking somehow to make sure that ordering based functions are not called for k != 2
  std::vector<std::pair<std::string, HypergraphCutFunc>> cut_funcs = {
      {"cxy", [k, cut_value](Hypergraph *h,
                             uint64_t seed,
                             hypergraph_util::ContractionStats &stats) {
        return cxy::discover_stats<Hypergraph, 0>(*h,
                                                  k,
                                                  cut_value,
                                                  stats,
                                                  seed);
      }},
      {"fpz", [k, cut_value](Hypergraph *h,
                             uint64_t seed,
                             hypergraph_util::ContractionStats &stats) {
        return fpz::discover_stats<Hypergraph, 0>(*h,
                                                  k,
                                                  cut_value,
                                                  stats,
                                                  seed);
      }},
      {"kk", [k, cut_value](Hypergraph *h,
                            uint64_t seed,
                            hypergraph_util::ContractionStats &stats) {
        return kk::discover_stats<Hypergraph, 0>(*h,
                                                 k,
                                                 cut_value,
                                                 stats,
                                                 seed);
      }},
      {"mw", [](Hypergraph *h,
                uint64_t seed,
                hypergraph_util::ContractionStats &stats) { return MW_min_cut<Hypergraph>(*h); }},
      {"q", [](Hypergraph *h,
               uint64_t seed,
               hypergraph_util::ContractionStats &stats) { return Q_min_cut<Hypergraph>(*h); }},
      {"kw", [](Hypergraph *h,
                uint64_t seed,
                hypergraph_util::ContractionStats &stats) { return KW_min_cut<Hypergraph>(*h); }}
  };
  cut_funcs.erase(std::remove_if(std::begin(cut_funcs),
                                 std::end(cut_funcs),
                                 [this](auto &f) { return notInFuncNames(f); }), std::end(cut_funcs));
  return cut_funcs;
}

std::vector<std::pair<std::string,
                      HypergraphCutValFunc>> DiscoveryRunner::getCutValAlgos(const HypergraphWrapper &hypergraph,
                                                                             const size_t k,
                                                                             const size_t cut_value) {
  std::vector<std::pair<std::string, HypergraphCutValFunc>> cutval_funcs = {
      {"cxyval", [k, cut_value](Hypergraph *h,
                                uint64_t seed,
                                hypergraph_util::ContractionStats &stats) {
        return cxy::discover_value<Hypergraph,
                                   0>(*h,
                                      k,
                                      cut_value,
                                      stats,
                                      seed);
      }},
      {"fpzval", [k, cut_value](Hypergraph *h,
                                uint64_t seed,
                                hypergraph_util::ContractionStats &stats) {
        return fpz::discover_value<Hypergraph,
                                   0>(*h,
                                      k,
                                      cut_value,
                                      stats,
                                      seed);
      }},
      {"kkval", [k, cut_value](Hypergraph *h,
                               uint64_t seed,
                               hypergraph_util::ContractionStats &stats) {
        return kk::discover_value<Hypergraph, 0>(*h,
                                                 k,
                                                 cut_value,
                                                 stats,
                                                 seed);
      }},
      {"mwval", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) {
        return MW_min_cut_value<Hypergraph>(*h);
      }},
      {"qval", [](Hypergraph *h,
                  uint64_t seed,
                  hypergraph_util::ContractionStats &stats) { return Q_min_cut_value<Hypergraph>(*h); }},
      {"kwval", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) {
        return KW_min_cut_value<Hypergraph>(*h);
      }}
  };
  cutval_funcs.erase(std::remove_if(std::begin(cutval_funcs),
                                    std::end(cutval_funcs),
                                    [this](auto &f) { return notInFuncNames(f); }), std::end(cutval_funcs));
  return cutval_funcs;
}

template<typename T>
bool DiscoveryRunner::notInFuncNames(T &&f) {
  if (funcnames_.empty()) {
    return false;
  }
  const auto &[name, func] = f;
  return std::find(std::cbegin(funcnames_), std::cend(funcnames_), name) == std::cend(funcnames_);
}

