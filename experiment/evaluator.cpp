//
// Created by Victor on 2/23/20.
//

#include <unistd.h>

#include "evaluator.hpp"
#include "source.hpp"
#include "store.hpp"

#include <hypergraph/cut.hpp>
#include <hypergraph/order.hpp>
#include <hypergraph/hypergraph.hpp>
#include <hypergraph/cxy.hpp>
#include <hypergraph/fpz.hpp>

namespace {

std::string hostname() {
  std::vector<char> hostname(50, '0');
  gethostname(hostname.data(), 49);
  return {hostname.data()};
}

// Visitor for discovery algorithms
struct DiscoverVisitor {
  using F = std::function<HypergraphCut<size_t>(const Hypergraph &, size_t, size_t, uint64_t)>;
  using WF = std::function<HypergraphCut<size_t>(const WeightedHypergraph<size_t> &, size_t, size_t, uint64_t)>;

  DiscoverVisitor(size_t k, size_t discovery_val, uint64_t seed, F f, WF wf)
      : k(k), discovery_val(discovery_val), seed(seed), f(f), wf(wf) {}

  size_t k;
  size_t discovery_val;
  uint64_t seed;
  std::function<HypergraphCut<size_t>(const Hypergraph &, size_t, size_t, uint64_t)> f;
  std::function<HypergraphCut<size_t>(const WeightedHypergraph<size_t> &, size_t, size_t, uint64_t)> wf;

  HypergraphCut<size_t> operator()(const Hypergraph &h) const {
    return f(h, k, discovery_val, seed);
  }

  HypergraphCut<size_t> operator()(const WeightedHypergraph<size_t> &h) const {
    return wf(h, k, discovery_val, seed);
  }
};

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

  CutRunInfo run_info(info);
  run_info.algorithm = "MW";
  run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  run_info.machine = hostname();
  run_info.commit = "n/a";

  const auto[status, cut_id] = store_->report(h.name, info);
  if (status == ReportStatus::ERROR) {
    std::cerr << "Error when reporting cut" << std::endl;
    return;
  }

  if (store_->report(h.name, cut_id, run_info) == ReportStatus::ERROR) {
    std::cerr << "Error when reporting run" << std::endl;
  }
}

KDiscoveryRunner::KDiscoveryRunner(std::unique_ptr<PlantedHypergraphSource> &&source,
                                   std::unique_ptr<CutInfoStore> &&store) :
    src_(std::move(source)), store_(std::move(store)) {}

void KDiscoveryRunner::run() {
  while (src_->has_next()) {
    auto[hypergraph, planted_cut] = src_->generate();
    std::cout << hypergraph.name << "..." << std::endl;

    if (store_->report(hypergraph) == ReportStatus::ERROR) {
      std::cout << "Failed to put hypergraph in DB" << std::endl;
      continue;
    }

    ReportStatus status;
    uint64_t planted_cut_id;
    std::tie(status, planted_cut_id) = store_->report(hypergraph.name, planted_cut);
    if (status == ReportStatus::ERROR) {
      std::cout << "Failed to put planted cut info in DB" << std::endl;
      continue;
    }

    // TODO Random seed
    DiscoverVisitor cxy_visit
        (planted_cut.k,
         planted_cut.cut_value,
         1337,
         static_cast<DiscoverVisitor::F>(cxy::discover<Hypergraph, 1>),
         static_cast<DiscoverVisitor::WF>(cxy::discover<WeightedHypergraph<size_t>, 1>));
    DiscoverVisitor fpz_visit
        (planted_cut.k,
         planted_cut.cut_value,
         1337,
         static_cast<DiscoverVisitor::F>(fpz::discover<Hypergraph, 1>),
         static_cast<DiscoverVisitor::WF>(fpz::discover<WeightedHypergraph<size_t>, 1>));

    auto compute_run =
        [this, planted_cut_id](const DiscoverVisitor &visitor,
                               std::string func_name,
                               const CutInfo &planted_cut,
                               const HypergraphWrapper &hypergraph) -> void {
          // Now take the times
          auto start = std::chrono::high_resolution_clock::now();
          HypergraphCut<size_t> cxy_cut = std::visit(visitor, hypergraph.h);
          auto stop = std::chrono::high_resolution_clock::now();

          CutInfo found_cut_info(planted_cut.k, cxy_cut);

          CutRunInfo run_info(found_cut_info);
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
              std::cout << "Found cut has same value as planted cut but different partitions" << std::endl;
            } else {
              std::cout << "Found cut value " << found_cut_info.cut_value << " < planted cut value "
                        << planted_cut.cut_value
                        << std::endl;
            }
            ReportStatus status;
            std::tie(status, found_cut_id) = store_->report(hypergraph.name, found_cut_info);
            if (status == ReportStatus::ERROR) {
              std::cout << "Failed to report found cut" << std::endl;
              return;
            }
          }

          if (store_->report(hypergraph.name, found_cut_id, run_info) == ReportStatus::ERROR) {
            std::cout << "Failed to report run" << std::endl;
          }
        };

    compute_run(cxy_visit, "cxy", planted_cut, hypergraph);
    compute_run(fpz_visit, "fpz", planted_cut, hypergraph);

  }
}

