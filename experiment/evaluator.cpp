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

  CutInfo info;

  info.hypergraph = h.name;
  info.k = 2;
  info.cut_value = cut.value;
  info.partitions = cut.partitions;

  std::vector<char> hostname(50, '0');
  gethostname(hostname.data(), 49);

  CutRunInfo run_info;
  run_info.algorithm = "MW";
  run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
  run_info.machine = std::string(hostname.data());
  run_info.commit = "n/a";
  run_info.info = info;

  const auto [status, cut_id] = store_->report(h.name, info);
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
    auto[hypergraph, cut] = src_->generate();
    std::cout << hypergraph.name << "..." << std::endl;
    std::cout << "Cut planted with value " << cut.cut_value << std::endl;
    std::cout << cut << std::endl;

    switch (store_->report(hypergraph)) {
    case ReportStatus::ALREADY_THERE: {
      std::cout << "Hypergraph already in DB" << std::endl;
      break;
    }
    case ReportStatus::ERROR: {
      std::cout << "Failed to put hypergraph in DB" << std::endl;
      continue;
    }
    case ReportStatus::OK: {
      // Do nothing...
    }
    }

    ReportStatus status;
    uint64_t cut_id;

    std::tie(status, cut_id) = store_->report(hypergraph.name, cut);
    switch (status) {
    case ReportStatus::ALREADY_THERE: {
      std::cout << "Cut info already in DB" << std::endl;
      return;
    }
    case ReportStatus::ERROR: {
      std::cout << "Failed to put cut info in DB" << std::endl;
      continue;
    }
    case ReportStatus::OK: {
      // Do nothing...
    }
    }

    // This could be probably done better
    struct DiscoverVisitor {
      using F = std::function<HypergraphCut<size_t>(const Hypergraph &, size_t, size_t, uint64_t)>;
      using WF = std::function<HypergraphCut<size_t>(const WeightedHypergraph <size_t> &, size_t, size_t, uint64_t)>;

      DiscoverVisitor(size_t k, size_t discovery_val, uint64_t seed, F f, WF wf)
          : k(k), discovery_val(discovery_val), seed(seed), f(f), wf(wf) {}

      size_t k;
      size_t discovery_val;
      uint64_t seed;
      std::function<HypergraphCut<size_t>(const Hypergraph &, size_t, size_t, uint64_t)> f;
      std::function<HypergraphCut<size_t>(const WeightedHypergraph <size_t> &, size_t, size_t, uint64_t)> wf;

      HypergraphCut<size_t> operator()(const Hypergraph &h) {
        return f(h, k, discovery_val, seed);
      }

      HypergraphCut<size_t> operator()(const WeightedHypergraph <size_t> &h) {
        return wf(h, k, discovery_val, seed);
      }
    } cxy_visit
        (cut.k, cut.cut_value, 1337, cxy::discover<Hypergraph, 1>, cxy::discover<WeightedHypergraph < size_t>, 1 > ),
        fpz_visit
        (cut.k, cut.cut_value, 1337, fpz::discover<Hypergraph, 1>, fpz::discover<WeightedHypergraph < size_t>, 1 > );
    // TODO Random seed

    // Now take the times
    auto start = std::chrono::high_resolution_clock::now();
    HypergraphCut<size_t> cxy_cut = std::visit(cxy_visit, hypergraph.h);
    auto stop = std::chrono::high_resolution_clock::now();

    // TODO put this in its own function
    std::vector<char> hostname(50, '0');
    gethostname(hostname.data(), 49);

    CutInfo info;
    info.hypergraph = hypergraph.name;
    info.k = cut.k;
    info.cut_value = cxy_cut.value;
    info.partitions = cxy_cut.partitions;

    CutRunInfo run_info;
    run_info.algorithm = "CXY";
    run_info.time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    run_info.machine = std::string(hostname.data());
    run_info.commit = "n/a";
    run_info.info = info;

    // Report computed cut info to see if it is the same as the planted cut
    // (Alternatively we can just compare them in memory because we already have both in memory
    // If they are not the same then we need to do something about it...
    // Report run
    std::tie(status, cut_id) = store_->report(hypergraph.name, info);
    store_->report(hypergraph.name, cut_id, run_info);
  }
}

