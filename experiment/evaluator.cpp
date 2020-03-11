//
// Created by Victor on 2/23/20.
//

#include <unistd.h>

#include "evaluator.hpp"
#include "source.hpp"
#include "store.hpp"
#include "generators.hpp"

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

}

CutEvaluator::CutEvaluator(std::unique_ptr<HypergraphSource> &&source, std::unique_ptr<CutInfoStore> &&store) : source_(
    std::move(source)), store_(std::move(store)) {}

void MinimumCutFinder::run() {
  std::cout << "Searching for minimum cuts" << std::endl;

  while (source_->has_next()) {
    evaluate();
  }

  std::cout << "Done finding minimum cuts" << std::endl;
}

void MinimumCutFinder::evaluate() {
  HypergraphWrapper h = source_->generate();

  std::cout << "Analyzing " << h.name << std::endl;

  switch (store_->report(h)) {
  case ReportStatus::ALREADY_THERE: {
    std::cout << "Already found minimum cut of this hypergraph" << std::endl;
    return;
  }
  case ReportStatus::ERROR: {
    std::cout << "Error when reporting hypergraph" << std::endl;
    return;
  }
  case ReportStatus::OK:
    // OK, continue;
    break;
  };

  struct {
    HypergraphCut<size_t> operator()(Hypergraph &h) { return MW_min_cut(h); }
    HypergraphCut<size_t> operator()(WeightedHypergraph<size_t> &h) { return MW_min_cut(h); }
  } mincut;

  auto start = std::chrono::high_resolution_clock::now();
  HypergraphCut<size_t> cut = std::visit(mincut, h.h);
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Done: took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
            << " milliseconds" << std::endl;
  if (cut.value > 0 && std::none_of(begin(cut.partitions), end(cut.partitions), [](const auto &p) {
    return p.size() == 1;
  })) {
    std::cout << "Found interesting cut for " << h.name << std::endl;
  }

  CutInfo info(2, cut);

  CutRunInfo run_info("MW_min_cut", info);
  run_info.algorithm = "MW";
  run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  run_info.machine = hostname();
  run_info.commit = "n/a";

  const auto[status, cut_id] = store_->report(h.name, info, false);
  if (status == ReportStatus::ERROR) {
    std::cerr << "Error when reporting cut" << std::endl;
    return;
  }

  if (store_->report(h.name, cut_id, run_info) == ReportStatus::ERROR) {
    std::cerr << "Error when reporting run" << std::endl;
  }
}

KDiscoveryRunner::KDiscoveryRunner(const std::string &id,
                                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                   std::unique_ptr<CutInfoStore> &&store, bool compare_kk) : id_(id),
                                                                                             src_(std::move(source)),
                                                                                             store_(std::move(store)),
                                                                                             compare_kk_(compare_kk) {}

void KDiscoveryRunner::run() {
  for (const auto &gen : src_) {
    const auto[hgraph, planted_cut_optional] = gen->generate();
    assert(planted_cut_optional.has_value());

    HypergraphWrapper hypergraph;
    hypergraph.h = hgraph;
    hypergraph.name = gen->name();

    const CutInfo planted_cut = planted_cut_optional.value();
    const size_t k = planted_cut.k;
    const size_t cut_value = planted_cut.cut_value;
    std::cout << hypergraph.name << "..." << std::endl;

    if (store_->report(*gen) == ReportStatus::ERROR) {
      std::cout << "Failed to put hypergraph in DB" << std::endl;
      continue;
    }

    ReportStatus status;
    uint64_t planted_cut_id;
    std::tie(status, planted_cut_id) = store_->report(hypergraph.name, planted_cut, true);
    if (status == ReportStatus::ERROR) {
      std::cout << "Failed to put planted cut info in DB" << std::endl;
      continue;
    }

    // Avoid this variant foolishness for now
    Hypergraph *hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);
    assert(hypergraph_ptr != nullptr);

    using HypergraphCutFunc = std::function<HypergraphCut<size_t>(Hypergraph *,
                                                                  uint64_t seed,
                                                                  hypergraph_util::ContractionStats &stats)>;
    using HypergraphCutValFunc = std::function<size_t(Hypergraph *,
                                                      uint64_t seed,
                                                      hypergraph_util::ContractionStats &stats)>;

    std::map<std::string, HypergraphCutFunc> cut_funcs = {
        {"cxy", [k, cut_value](Hypergraph *h,
                               uint64_t seed,
                               hypergraph_util::ContractionStats &stats) {
          return cxy::discover_stats<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
        {"fpz", [k, cut_value](Hypergraph *h,
                               uint64_t seed,
                               hypergraph_util::ContractionStats &stats) {
          return fpz::discover_stats<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
    };

    std::map<std::string, HypergraphCutValFunc> cutval_funcs = {
        {"cxyval", [k, cut_value](Hypergraph *h,
                                  uint64_t seed,
                                  hypergraph_util::ContractionStats &stats) {
          return cxy::discover_value<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
        {"fpzval", [k, cut_value](Hypergraph *h,
                                  uint64_t seed,
                                  hypergraph_util::ContractionStats &stats) {
          return fpz::discover_value<Hypergraph, 1>(*h,
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

    if (compare_kk_) {
      cut_funcs.insert({"kk", [k, cut_value](Hypergraph *h,
                                             uint64_t seed,
                                             hypergraph_util::ContractionStats &stats) {
        return kk::discover_stats<Hypergraph,
                                  1>(*h, k, cut_value, stats, seed);
      }});
      cutval_funcs.insert({"kkval", [k, cut_value](Hypergraph *h,
                                                   uint64_t seed,
                                                   hypergraph_util::ContractionStats &stats) {
        return kk::discover_value<Hypergraph,
                                  1>(*h, k, cut_value, stats, seed);
      }});
    }

    // Do them in order
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    for (int i = 0; i < 20; ++i) {
      for (const auto &[func_name, func] : cut_funcs) {
        Hypergraph temp(*hypergraph_ptr);
        std::cout << "Running " << func_name << " on " << hypergraph.name << std::endl;
        hypergraph_util::ContractionStats stats;

        auto start = std::chrono::high_resolution_clock::now();
        HypergraphCut<size_t> cut = func(&temp, dis(rgen), stats);
        auto stop = std::chrono::high_resolution_clock::now();

        CutInfo found_cut_info(k, cut);

        CutRunInfo run_info(id_, found_cut_info);
        run_info.algorithm = func_name;
        run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        run_info.machine = hostname();
        run_info.commit = "n/a";

        // Check if found cut was the planted cut
        uint64_t found_cut_id;
        if (found_cut_info == planted_cut) {
          std::cout << "Found the planted cut" << std::endl;
          found_cut_id = planted_cut_id;
        } else {
          // Otherwise we need to report this cut to the DB to get its ID
          if (found_cut_info.cut_value == planted_cut.cut_value) {
            std::cout << "Found cut has same value as planted cut (" << planted_cut.cut_value
                      << ") but different partitions:";
            for (const auto &partition : found_cut_info.partitions) {
              std::cout << " " << partition.size();
            }
            std::cout << std::endl;
          } else {
            std::cout << "Found cut value " << found_cut_info.cut_value << " < planted cut value "
                      << planted_cut.cut_value
                      << std::endl;
          }
          ReportStatus status;
          std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut_info, false);
          if (status == ReportStatus::ERROR) {
            std::cout << "Failed to report found cut" << std::endl;
            return;
          }
        }

        if (store_->report(hypergraph.name,
                           found_cut_id,
                           run_info,
                           stats.num_runs,
                           stats.num_contractions)
            == ReportStatus::ERROR) {
          std::cout << "Failed to report run" << std::endl;
        }
      };

      for (const auto &[func_name, func] : cutval_funcs) {
        Hypergraph temp(*hypergraph_ptr);
        std::cout << "Running " << func_name << " on " << hypergraph.name << std::endl;
        hypergraph_util::ContractionStats stats;
        auto start = std::chrono::high_resolution_clock::now();
        size_t cutval = func(&temp, dis(rgen), stats);
        auto stop = std::chrono::high_resolution_clock::now();

        if (cutval < planted_cut.cut_value) {
          std::cout << "Found cut value has lesser value than planted cut" << std::endl;
        }

        CutInfo found_cut_info(k, cut_value);

        CutRunInfo run_info(id_, found_cut_info);
        run_info.algorithm = func_name;
        run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        run_info.machine = hostname();
        run_info.commit = "n/a";

        if (store_->report(hypergraph.name,
                           run_info,
                           stats.num_runs,
                           stats.num_contractions)
            == ReportStatus::ERROR) {
          std::cout << "Failed to report run" << std::endl;
        }
      }
    }
  }
}

RandomRingRunner::RandomRingRunner(const std::string &id,
                                   std::vector<std::unique_ptr<HypergraphGenerator>> &&source,
                                   std::unique_ptr<CutInfoStore> &&store, bool compare_kk) : id_(id),
                                                                                             src_(std::move(source)),
                                                                                             store_(std::move(store)),
                                                                                             compare_kk_(compare_kk) {}

void RandomRingRunner::run() {
  for (const auto &gen : src_) {
    // We need to initially check that the hypergraph has an interesting cut...
    const auto[hgraph, planted_cut_optional] = gen->generate();
    assert(!planted_cut_optional.has_value());

    HypergraphWrapper hypergraph;
    hypergraph.h = hgraph;
    hypergraph.name = gen->name();

    // Avoid this variant foolishness for now
    const Hypergraph *const hypergraph_ptr = std::get_if<Hypergraph>(&hypergraph.h);
    Hypergraph temp(*hypergraph_ptr);

    assert(hypergraph_ptr != nullptr);

    CutInfo planted_cut(2, MW_min_cut(temp));

    std::cout << hypergraph.name << ": size " << hypergraph_ptr->size() << std::endl;

    if (planted_cut.cut_value == 0) {
      std::cout << hypergraph.name << " is uninteresting: cut value is 0" << std::endl;
      continue;
    }

    if (std::any_of(std::begin(planted_cut.partitions), std::end(planted_cut.partitions), [](const auto &p) {
      return p.size() == 1;
    })) {
      std::cout << hypergraph.name << " is uninteresting: skewed partitions" << std::endl;
      continue;
    }

    std::cout << hypergraph.name << " is interesting: cut value is "
              << planted_cut.cut_value
              << ", partitions have sizes " << planted_cut.partitions[0].size()
              << " and "
              << planted_cut.partitions[1].size()
              << std::endl;

    const size_t k = planted_cut.k;
    const size_t cut_value = planted_cut.cut_value;
    std::cout << hypergraph.name << "..." << std::endl;

    if (store_->report(*gen) == ReportStatus::ERROR) {
      std::cout << "Failed to put hypergraph in DB" << std::endl;
      continue;
    }

    ReportStatus status;
    uint64_t planted_cut_id;
    std::tie(status, planted_cut_id) = store_->report(hypergraph.name, planted_cut, true);
    if (status == ReportStatus::ERROR) {
      std::cout << "Failed to put planted cut info in DB" << std::endl;
      continue;
    }


    using HypergraphCutFunc = std::function<HypergraphCut<size_t>(Hypergraph *,
                                                                  uint64_t seed,
                                                                  hypergraph_util::ContractionStats &stats)>;
    using HypergraphCutValFunc = std::function<size_t(Hypergraph *,
                                                      uint64_t seed,
                                                      hypergraph_util::ContractionStats &stats)>;

    std::map<std::string, HypergraphCutFunc> cut_funcs = {
        {"cxy", [k, cut_value](Hypergraph *h,
                               uint64_t seed,
                               hypergraph_util::ContractionStats &stats) {
          return cxy::discover_stats<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
        {"fpz", [k, cut_value](Hypergraph *h,
                               uint64_t seed,
                               hypergraph_util::ContractionStats &stats) {
          return fpz::discover_stats<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
    };

    std::map<std::string, HypergraphCutValFunc> cutval_funcs = {
        {"cxyval", [k, cut_value](Hypergraph *h,
                                  uint64_t seed,
                                  hypergraph_util::ContractionStats &stats) {
          return cxy::discover_value<Hypergraph, 1>(*h,
                                                    k,
                                                    cut_value,
                                                    stats,
                                                    seed);
        }},
        {"fpzval", [k, cut_value](Hypergraph *h,
                                  uint64_t seed,
                                  hypergraph_util::ContractionStats &stats) {
          return fpz::discover_value<Hypergraph, 1>(*h,
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

    if (compare_kk_) {
      cut_funcs.insert({"kk", [k, cut_value](Hypergraph *h,
                                             uint64_t seed,
                                             hypergraph_util::ContractionStats &stats) {
        return kk::discover_stats<Hypergraph,
                                  1>(*h, k, cut_value, stats, seed);
      }});
      cutval_funcs.insert({"kkval", [k, cut_value](Hypergraph *h,
                                                   uint64_t seed,
                                                   hypergraph_util::ContractionStats &stats) {
        return kk::discover_value<Hypergraph,
                                  1>(*h, k, cut_value, stats, seed);
      }});
    }

    // Do them in order
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    for (int i = 0; i < 20; ++i) {
      for (const auto &[func_name, func] : cut_funcs) {
        Hypergraph temp(*hypergraph_ptr);
        std::cout << "Running " << func_name << " on " << hypergraph.name << std::endl;
        hypergraph_util::ContractionStats stats;

        auto start = std::chrono::high_resolution_clock::now();
        HypergraphCut<size_t> cut = func(&temp, dis(rgen), stats);
        auto stop = std::chrono::high_resolution_clock::now();

        CutInfo found_cut_info(k, cut);

        CutRunInfo run_info(id_, found_cut_info);
        run_info.algorithm = func_name;
        run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        run_info.machine = hostname();
        run_info.commit = "n/a";

        // Check if found cut was the planted cut
        uint64_t found_cut_id;
        if (found_cut_info == planted_cut) {
          std::cout << "Found the planted cut" << std::endl;
          found_cut_id = planted_cut_id;
        } else {
          // Otherwise we need to report this cut to the DB to get its ID
          if (found_cut_info.cut_value == planted_cut.cut_value) {
            std::cout << "Found cut has same value as planted cut (" << planted_cut.cut_value
                      << ") but different partitions:";
            for (const auto &partition : found_cut_info.partitions) {
              std::cout << " " << partition.size();
            }
            std::cout << std::endl;
          } else {
            std::cout << "Found cut value " << found_cut_info.cut_value << " < planted cut value "
                      << planted_cut.cut_value
                      << std::endl;
          }
          ReportStatus status;
          std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut_info, false);
          if (status == ReportStatus::ERROR) {
            std::cout << "Failed to report found cut" << std::endl;
            return;
          }
        }

        if (store_->report(hypergraph.name,
                           found_cut_id,
                           run_info,
                           stats.num_runs,
                           stats.num_contractions)
            == ReportStatus::ERROR) {
          std::cout << "Failed to report run" << std::endl;
        }
      };

      for (const auto &[func_name, func] : cutval_funcs) {
        Hypergraph temp(*hypergraph_ptr);
        std::cout << "Running " << func_name << " on " << hypergraph.name << std::endl;
        hypergraph_util::ContractionStats stats;
        auto start = std::chrono::high_resolution_clock::now();
        size_t cutval = func(&temp, dis(rgen), stats);
        auto stop = std::chrono::high_resolution_clock::now();

        if (cutval < planted_cut.cut_value) {
          std::cout << "Found cut value has lesser value than planted cut" << std::endl;
        }

        CutInfo found_cut_info(k, cut_value);

        CutRunInfo run_info(id_, found_cut_info);
        run_info.algorithm = func_name;
        run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        run_info.machine = hostname();
        run_info.commit = "n/a";

        if (store_->report(hypergraph.name,
                           run_info,
                           stats.num_runs,
                           stats.num_contractions)
            == ReportStatus::ERROR) {
          std::cout << "Failed to report run" << std::endl;
        }
      }
    }
  }
}
