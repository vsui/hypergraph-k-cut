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

  uint64_t cut_id;
  if (store_->report(info, cut_id) == ReportStatus::ERROR) {
    std::cerr << "Error when reporting cut" << std::endl;
  }
  run_info.info.id = cut_id;

  if (store_->report(run_info) == ReportStatus::ERROR) {
    std::cerr << "Error when reporting run" << std::endl;
  }
}

