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

using HypergraphCutFunc = std::function<HypergraphCut<size_t>(Hypergraph *,
                                                              uint64_t,
                                                              hypergraph_util::ContractionStats &)>;

using HypergraphCutValFunc = std::function<size_t(Hypergraph *, uint64_t, hypergraph_util::ContractionStats &)>;

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

// This should probably be a private method of ExperimentRunner but it would be annoying to have to change the inferface
// to include all of the arguments.
template<bool ReturnsPartitions>
void run(const std::unique_ptr<HypergraphGenerator> &gen,
         const std::string &func_name,
         typename CutFunc<ReturnsPartitions>::T func,
         CutInfoStore *store_,
         size_t num_runs_,
         const std::string &id_,
         const CutInfo &planted_cut,
         const uint64_t planted_cut_id,
         std::mt19937_64 &rgen,
         std::uniform_int_distribution<uint64_t> &dis, size_t k) {
  const auto[hgraph, planted_cut_optional] = gen->generate();
  HypergraphWrapper hypergraph;
  hypergraph.h = hgraph;
  hypergraph.name = gen->name();
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
      spdlog::warn("Found cut has lesser value than the planted cut");

      if (store_->report(hypergraph.name,
                         run_info,
                         stats.num_runs,
                         stats.num_contractions)
          == ReportStatus::ERROR) {
        spdlog::error("Failed to report run");
      }
    }
  };
}

std::map<std::string, HypergraphCutFunc> cut_funcs(const size_t k, const bool add_kk, const size_t cut_value) {
  std::map<std::string, HypergraphCutFunc> cut_funcs = {
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
  };

  if (k == 2) {
    cut_funcs.insert({"mw", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) {
      return MW_min_cut<Hypergraph>(*h);
    }});
    cut_funcs.insert({"q", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) {
      return Q_min_cut<Hypergraph>(*h);
    }});
    cut_funcs.insert({"kw", [](Hypergraph *h, uint64_t seed, hypergraph_util::ContractionStats &stats) {
      return KW_min_cut<Hypergraph>(*h);
    }});
  }

  if (add_kk) {
    cut_funcs.insert({"kk", [k, cut_value](Hypergraph *h,
                                           uint64_t seed,
                                           hypergraph_util::ContractionStats &stats) {
      return kk::discover_stats<Hypergraph,
                                0>(*h, k, cut_value, stats, seed);
    }});
  }
  return cut_funcs;
}

std::map<std::string, HypergraphCutValFunc> cutval_funcs(const size_t k, const bool add_kk, const size_t cut_value) {
  std::map<std::string, HypergraphCutValFunc> cutval_funcs = {
      {"cxyval", [k, cut_value](Hypergraph *h,
                                uint64_t seed,
                                hypergraph_util::ContractionStats &stats) {
        return cxy::discover_value<Hypergraph, 0>(*h,
                                                  k,
                                                  cut_value,
                                                  stats,
                                                  seed);
      }},
      {"fpzval", [k, cut_value](Hypergraph *h,
                                uint64_t seed,
                                hypergraph_util::ContractionStats &stats) {
        return fpz::discover_value<Hypergraph, 0>(*h,
                                                  k,
                                                  cut_value,
                                                  stats,
                                                  seed);
      }},
  };
  if (k == 2) {
    cutval_funcs.insert({"mwval", [](Hypergraph *h,
                                     uint64_t seed,
                                     hypergraph_util::ContractionStats &stats) { return MW_min_cut_value<Hypergraph>(*h); }});
    cutval_funcs.insert({"qval", [](Hypergraph *h,
                                    uint64_t seed,
                                    hypergraph_util::ContractionStats &stats) { return Q_min_cut_value<Hypergraph>(*h); }});
    cutval_funcs.insert({"kwval", [](Hypergraph *h,
                                     uint64_t seed,
                                     hypergraph_util::ContractionStats &stats) { return KW_min_cut_value<Hypergraph>(*h); }});
  }
  if (add_kk) {
    cutval_funcs.insert({"kkval", [k, cut_value](Hypergraph *h,
                                                 uint64_t seed,
                                                 hypergraph_util::ContractionStats &stats) {
      return kk::discover_value<Hypergraph,
                                0>(*h, k, cut_value, stats, seed);
    }});
  }
  return cutval_funcs;
}

}

ExperimentRunner::ExperimentRunner(const std::string &id,
                                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                   std::shared_ptr<CutInfoStore> store,
                                   bool compare_kk,
                                   bool planted,
                                   size_t num_runs) : id_(id),
                                                      src_(std::move(source)),
                                                      store_(std::move(store)),
                                                      compare_kk_(compare_kk),
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
    // Do initial processing (put hypergraph in DB)
    const auto[hgraph, planted_cut_optional] = gen->generate();
    HypergraphWrapper hypergraph;
    hypergraph.h = hgraph;
    hypergraph.name = gen->name();
    // Avoid this variant foolishness for now
    Hypergraph *hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);
    assert(hypergraph_ptr != nullptr);
    Hypergraph temp(*hypergraph_ptr);

    ReportStatus status = store_->report(hypergraph);
    if (status == ReportStatus::ERROR) {
      spdlog::error("Failed to put hypergraph info in DB");
      continue;
    }

    const CutInfo planted_cut = planted_ ? planted_cut_optional.value() : CutInfo{2, MW_min_cut(temp)};

    if (!planted_) {
      // Skip if the cut is uninteresting or skewed
      if (planted_cut.cut_value == 0) {
        spdlog::warn("Skipping {}, hypergraph is disconnected", hypergraph.name);
        continue;
      }
      // We are assuming here that there are two partitions since as of now there is no way to get a cut efficiently
      // where k>2
      double_t skew = static_cast<double>(planted_cut.partitions[0].size()) / hgraph.num_vertices();
      if (skew < 0.1 || skew > 0.9) {
        spdlog::warn("Skipping {}, cut is skewed ({}, {})",
                     hypergraph.name,
                     planted_cut.partitions.at(0).size(),
                     planted_cut.partitions.at(1).size());
        continue;
      }
    }

    const size_t k = planted_cut.k;
    const size_t cut_value = planted_cut.cut_value;

    uint64_t planted_cut_id;
    std::tie(status, planted_cut_id) = store_->report(hypergraph.name, planted_cut, true);
    if (status == ReportStatus::ERROR) {
      spdlog::error("Failed to put planted cut info in DB");
      continue;
    }

    // Do them in order
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    spdlog::info("[{}] Collecting data for hypergraph", hypergraph.name);
    for (const auto &[func_name, func] : cut_funcs(k, compare_kk_, cut_value)) {
      ::template run<true>(gen,
                           func_name,
                           func,
                           store_.get(),
                           num_runs_,
                           id_,
                           planted_cut,
                           planted_cut_id,
                           rgen,
                           dis,
                           k);
    }
    for (const auto &[func_name, func] : cutval_funcs(k, compare_kk_, cut_value)) {
      ::template run<false>(gen,
                            func_name,
                            func,
                            store_.get(),
                            num_runs_,
                            id_,
                            planted_cut,
                            planted_cut_id,
                            rgen,
                            dis,
                            k);
    }
  }
}

