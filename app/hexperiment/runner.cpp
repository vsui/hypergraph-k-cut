//
// Created by Victor on 2/23/20.
//

#include <unistd.h>

#include "runner.hpp"
#include "store.hpp"
#include "generators.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <hypergraph/cut.hpp>
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

std::vector<std::pair<std::string, HypergraphCutFunc>> cut_funcs(const size_t k, const size_t cut_value) {
  //@formatter:off
  std::vector<std::pair<std::string, HypergraphCutFunc>> cut_funcs = {
      {"cxy", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return cxy::discover_stats<Hypergraph, 0>(*h, k, cut_value, stats, seed); }},
      {"fpz", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return fpz::discover_stats<Hypergraph, 0>(*h, k, cut_value, stats, seed); }},
      {"kk", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return kk::discover_stats<Hypergraph, 0>(*h, k, cut_value, stats, seed); }}
  };
  if (k == 2) {
    cut_funcs.push_back({"mw", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return MW_min_cut<Hypergraph>(*h); }});
    cut_funcs.push_back({"q", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return Q_min_cut<Hypergraph>(*h); }});
    cut_funcs.push_back({"kw", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return KW_min_cut<Hypergraph>(*h); }});
  }
  //@formatter.on
  return cut_funcs;
}

std::vector<std::pair<std::string, HypergraphCutValFunc>> cutval_funcs(const size_t k, const size_t cut_value) {
  //@formatter:off
  std::vector<std::pair<std::string, HypergraphCutValFunc>> cutval_funcs = {
      {"cxyval", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return cxy::discover_value<Hypergraph, 0>(*h, k, cut_value, stats, seed); }},
      {"fpzval", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return fpz::discover_value<Hypergraph, 0>(*h, k, cut_value, stats, seed); }},
      {"kkval", [k, cut_value](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return kk::discover_value<Hypergraph, 0>(*h, k, cut_value, stats, seed); }}
  };
  if (k == 2) {
    cutval_funcs.push_back({"mwval", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return MW_min_cut_value<Hypergraph>(*h); }});
    cutval_funcs.push_back({"qval", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return Q_min_cut_value<Hypergraph>(*h); }});
    cutval_funcs.push_back({"kwval", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return KW_min_cut_value<Hypergraph>(*h); }});
  }
  //@formatter:on
  return cutval_funcs;
}

std::vector<std::pair<std::string, HypergraphCutValFunc>> cutval_cutoff_funcs(const size_t mw_time,
                                                                              const size_t discovery_val,
                                                                              const std::vector<size_t> &cutoff_percentages) {
  std::vector<std::pair<std::string, HypergraphCutValFunc>> funcs;
  //@formatter:off
  for (const auto percentage : cutoff_percentages) {
    funcs.push_back({"cxyval_cutoff_" + std::to_string(percentage), [percentage, mw_time, discovery_val](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return hypergraph_util::repeat_contraction<Hypergraph, cxy::CxyImpl, false, 0>(*h, 2, std::mt19937_64{seed}, stats, std::nullopt, {discovery_val}, {(static_cast<double>(percentage) / 100) * mw_time}); }});
    funcs.push_back({"fpzval_cutoff_" + std::to_string(percentage), [percentage, mw_time, discovery_val](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return hypergraph_util::repeat_contraction<Hypergraph, fpz::FpzImpl, false, 0>(*h, 2, std::mt19937_64{seed}, stats, std::nullopt, {discovery_val}, {(static_cast<double>(percentage) / 100) * mw_time}); }});
    funcs.push_back({"kkval_cutoff_" + std::to_string(percentage), [percentage, mw_time, discovery_val](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) { return hypergraph_util::repeat_contraction<Hypergraph, kk::KkImpl, false, 0>(*h, 2, std::mt19937_64{seed}, stats, std::nullopt, {discovery_val}, {(static_cast<double>(percentage) / 100) * mw_time}); }});
  }
  //@formatter:on
  return funcs;
}

}

ExperimentRunner::ExperimentRunner(const std::string &id,
                                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                   std::shared_ptr<CutInfoStore> store,
                                   bool planted,
                                   bool cutoff,
                                   size_t num_runs) : id_(id),
                                                      src_(std::move(source)),
                                                      store_(std::move(store)),
                                                      planted_(planted),
                                                      cutoff_(cutoff),
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


    // Run experiment
    // Preliminary work done, start experiments
    if (cutoff_) {
      spdlog::info("{}: Calculating cutoff time", hypergraph.name);
      double mw_time = 0;
      for (int i = 0; i < num_runs_; ++i) {
        // TODO make this unnecessary
        // Avoid this variant foolishness for now
        const Hypergraph *hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);

        Hypergraph temp(*hypergraph_ptr);
        auto start = std::chrono::high_resolution_clock::now();
        auto cut = MW_min_cut_value(temp);
        auto stop = std::chrono::high_resolution_clock::now();
        size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        mw_time += ms;
        spdlog::info("[{}/{}] took {} milliseconds", i + 1, num_runs_, ms);
      }
      mw_time /= num_runs_;
      spdlog::info("Cutoff time is {}", mw_time);

      for (const auto &[func_name, func] : cutval_cutoff_funcs(mw_time, cut_value, cutoff_percentages_)) {
        doRun<false, true>(*gen, func_name, func, planted_cut, planted_cut_id, k);
      }
      continue;
    }
    spdlog::info("[{}] Collecting data for hypergraph", hypergraph.name);

    auto cut_fs = cut_funcs(k, cut_value);
    cut_fs.erase(std::remove_if(std::begin(cut_fs), std::end(cut_fs), [this](auto &f) {
      const auto &[name, func] = f;
      return std::find(std::cbegin(funcnames_), std::cend(funcnames_), name) == std::end(funcnames_);
    }));

    auto cutval_fs = cutval_funcs(k, cut_value);
    cutval_fs.erase(std::remove_if(std::begin(cutval_fs), std::end(cutval_fs), [this](auto &f) {
      const auto &[name, func] = f;
      return std::find(std::cbegin(funcnames_), std::cend(funcnames_), name) == std::end(funcnames_);
    }));

    for (const auto &[func_name, func] : cut_fs) {
      doRun<true>(*gen, func_name, func, planted_cut, planted_cut_id, k);
    }
    for (const auto &[func_name, func] : cutval_fs) {
      doRun<false>(*gen, func_name, func, planted_cut, planted_cut_id, k);
    }
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

  const auto k = planted_cut.k;
  const auto cut_value = planted_cut.cut_value;

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

template<bool ReturnsPartitions, bool CutOff>
void ExperimentRunner::doRun(const HypergraphGenerator &gen,
                             const std::string &func_name,
                             typename CutFunc<ReturnsPartitions>::T func,
                             const CutInfo &planted_cut,
                             const uint64_t planted_cut_id,
                             size_t k) {
  static_assert(!CutOff || (CutOff && !ReturnsPartitions));

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

  Hypergraph temp(*hypergraph_ptr);
  spdlog::info("[{} / {}] Starting", hypergraph.name, func_name);
  for (int i = 0; i < num_runs_; ++i) {
    spdlog::info("[{} / {}] Run {}/{}", hypergraph.name, func_name, i + 1, num_runs_);

    // We have to make a copy of the hypergraph since some algorithms write to it
    Hypergraph temp(*hypergraph_ptr);
    hypergraph_util::ContractionStats stats{};

    auto start = std::chrono::high_resolution_clock::now();
    auto cut = func(&temp, dis(rgen), stats);
    auto stop = std::chrono::high_resolution_clock::now();

    CutInfo found_cut_info(k, cut);

    CutRunInfo run_info(id_, found_cut_info);
    run_info.algorithm = func_name;
    run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    run_info.machine = hostname();
    run_info.commit = "n/a";

    // doReportCut
    if constexpr (ReturnsPartitions) {
      // Check if found cut was the planted cut
      uint64_t found_cut_id;
      if (found_cut_info == planted_cut) {
        found_cut_id = planted_cut_id;
      } else {
        // Otherwise we need to report this cut to the DB to get its ID
        if (found_cut_info.cut_value == planted_cut.cut_value) {
          spdlog::warn("Found cut has same value as planted cut ({}) but for different partition sizes");
        } else {
          spdlog::warn("Found cut value {} is not the planted cut value {}",
                       found_cut_info.cut_value,
                       planted_cut.cut_value);
        }
        ReportStatus status;
        std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut_info, false);
        if (status == ReportStatus::ERROR) {
          spdlog::error("Failed to report found cut");
          return;
        }
      }

      if (store_->report(hypergraph.name,
                         found_cut_id,
                         run_info,
                         stats.num_runs,
                         stats.num_contractions)
          == ReportStatus::ERROR) {
        spdlog::error("Failed to report run");
      }
    } else {
      // Check if found cut value was lower than the planted cut
      if (cut < planted_cut.cut_value) {
        spdlog::warn("Found cut has lesser value than the planted cut");
      }

      if (cut > planted_cut.cut_value) {
        if constexpr (CutOff) {
          spdlog::info("Cut off at {} {}", cut, planted_cut.cut_value);
        } else {
          spdlog::error("Found cut has greater value than planted cut (should not be possible)");
        }
      }

      ReportStatus status;
      uint64_t found_cut_id;
      std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut_info, false);
      if (status == ReportStatus::ERROR) {
        spdlog::error("Failed to report found cut");
        return;
      }

      if (store_->report(hypergraph.name,
                         found_cut_id,
                         run_info,
                         stats.num_runs,
                         stats.num_contractions)
          == ReportStatus::ERROR) {
        spdlog::error("Failed to report run");
      }
    }
  };
}

