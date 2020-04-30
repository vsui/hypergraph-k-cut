#pragma once

#include <hypergraph/kk.hpp>
#include <hypergraph/approx.hpp>
#include <hypergraph/cxy.hpp>
#include <hypergraph/fpz.hpp>

using namespace hypergraphlib;

struct Options {
  std::string algorithm;
  std::string filename;
  size_t k = 2; // Compute k-cut

  std::optional<double> epsilon; // Epsilon for approximation algorithms

  // These options are for contraction algorithms
  std::optional<size_t> runs; // Number of runs to repeat contraction algo for
  std::optional<double> discover; // Discovery value
  uint32_t random_seed = 0;
  uint8_t verbosity = 2; // Verbose output
};

/**
 * A hypergraph cut function with all of the necessary arguments baked in except for the hypergraph. So this may be a
 * k-cut function where k > 2, or an approximation algorithm
 *
 * It takes in a reference to a hypergraph (so it may modify the hypergraph!) and returns a cut with partitions
 */
template<typename HypergraphType>
using CutFunc = std::function<HypergraphCut<typename HypergraphType::EdgeWeight>(HypergraphType &)>;

template<typename HypergraphType>
struct CutFuncBuilder {
  using Ptr = std::shared_ptr<CutFuncBuilder>;

  std::string name() { return name_; }

  /// Throw `std::invalid_argument` if options are not valid
  virtual void check(const Options &options) = 0;

  virtual CutFunc<HypergraphType> build(const Options &options) = 0;

  virtual ~CutFuncBuilder() = default;

  explicit CutFuncBuilder(std::string name) : name_(std::move(name)) {}

private:
  std::string name_;
};

template<typename HypergraphType, typename ContractImpl>
struct ContractionFuncBuilder : CutFuncBuilder<HypergraphType> {
  using CutFuncBuilder<HypergraphType>::CutFuncBuilder;

  void check(const Options &options) override {
    if (options.k < 2) {
      throw std::invalid_argument("k cannot be less than 2");
    }
    if (options.epsilon) {
      throw std::invalid_argument("epsilon not valid for contraction algos");
    }
  }

  CutFunc<HypergraphType> build(const Options &options) override {
    if (options.verbosity == 0) {
      return [options](HypergraphType &h) {
        util::ContractionStats stats;
        return util::repeat_contraction<HypergraphType, ContractImpl, true, 0>(h,
                                                                               options.k,
                                                                               std::mt19937_64(options.random_seed),
                                                                               stats,
                                                                               options.runs,
                                                                               options.discover);
      };
    } else if (options.verbosity == 1) {
      return [options](HypergraphType &h) {
        util::ContractionStats stats;
        return util::repeat_contraction<HypergraphType, ContractImpl, true, 1>(h,
                                                                               options.k,
                                                                               std::mt19937_64(options.random_seed),
                                                                               stats,
                                                                               options.runs,
                                                                               options.discover);
      };
    } else {
      return [options](HypergraphType &h) {
        util::ContractionStats stats;
        return util::repeat_contraction<HypergraphType, ContractImpl, true, 2>(h,
                                                                               options.k,
                                                                               std::mt19937_64(options.random_seed),
                                                                               stats,
                                                                               options.runs,
                                                                               options.discover);
      };
    }
  }
};

template<typename HypergraphType, ordering_t<HypergraphType> Ordering>
struct OrderingBasedMinCutFuncBuilder : CutFuncBuilder<HypergraphType> {
  using CutFuncBuilder<HypergraphType>::CutFuncBuilder;

  void check(const Options &options) override {
    if (options.k != 2) {
      throw std::invalid_argument("k must be 2 for ordering based min cut");
    }
    if (options.epsilon) {
      throw std::invalid_argument("epsilon option not valid for ordering based min cut");
    }
    if (options.runs) {
      throw std::invalid_argument("runs option not valid for ordering based min cut");
    }
    if (options.discover) {
      throw std::invalid_argument("discovery option not valid for ordering based min cut");
    }
    // TODO random_seed, verbosity
  }

  CutFunc<HypergraphType> build(const Options &options) override {
    return [](HypergraphType &hypergraph) {
      return vertex_ordering_mincut<HypergraphType, Ordering, true>(hypergraph);
    };
  }
};

template<typename HypergraphType>
struct CXMinCutBuilder : CutFuncBuilder<HypergraphType> {
  using CutFuncBuilder<HypergraphType>::CutFuncBuilder;

  void check(const Options &options) override {
    if (options.k != 2) {
      throw std::invalid_argument("k must be 2 for ordering based min cut");
    }
    if (options.epsilon) {
      throw std::invalid_argument("epsilon option not valid for ordering based min cut");
    }
    if (options.runs) {
      throw std::invalid_argument("runs option not valid for ordering based min cut");
    }
    if (options.discover) {
      throw std::invalid_argument("discovery option not valid for ordering based min cut");
    }
// TODO random_seed, verbosity
  }

  CutFunc<HypergraphType> build(const Options &options) override {
    return [options](HypergraphType &hypergraph) {
      return certificate_minimum_cut<HypergraphType, true>(hypergraph, MW_min_cut<HypergraphType>);
    };
  }
};

template<typename HypergraphType, auto Func>
struct ApproxMinCutBuilder : CutFuncBuilder<HypergraphType> {
  using CutFuncBuilder<HypergraphType>::CutFuncBuilder;

  void check(const Options &options) override {
    if (options.k != 2) {
      throw std::invalid_argument("k must be 2");
    }
    if (!options.epsilon) {
      throw std::invalid_argument("epsilon required");
    }
    if (options.runs) {
      throw std::invalid_argument("runs option not valid");
    }
    if (options.discover) {
      throw std::invalid_argument("discovery option not valid");
    }
  }

  CutFunc<HypergraphType> build(const Options &options) override {
    const double epsilon = options.epsilon.value();
    return [epsilon](HypergraphType &hypergraph) {
      return Func(hypergraph, epsilon);
    };
  }
};

template<typename HypergraphType>
const std::vector<typename CutFuncBuilder<HypergraphType>::Ptr> cut_funcs = {
    std::make_shared<ContractionFuncBuilder<HypergraphType, cxy>>("CXY"),
    std::make_shared<ContractionFuncBuilder<HypergraphType, fpz>>("FPZ"),
    std::make_shared<ContractionFuncBuilder<HypergraphType, kk>>("KK"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<HypergraphType, tight_ordering>>("MW"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<HypergraphType, queyranne_ordering>>("Q"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<HypergraphType, maximum_adjacency_ordering>>("KW"),
    std::make_shared<ApproxMinCutBuilder<HypergraphType, approximate_minimizer<HypergraphType>>>("apxCX")
};

// Specialization: certificates are not supported by weighted hypergraphs yet
// TODO there should probably some kind of warning if someone tries to run CX on a weighted hypergraph
template<>
const std::vector<typename CutFuncBuilder<Hypergraph>::Ptr> cut_funcs<Hypergraph> = {
    std::make_shared<ContractionFuncBuilder<Hypergraph, cxy>>("CXY"),
    std::make_shared<ContractionFuncBuilder<Hypergraph, fpz>>("FPZ"),
    std::make_shared<ContractionFuncBuilder<Hypergraph, kk>>("KK"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<Hypergraph, tight_ordering>>("MW"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<Hypergraph, queyranne_ordering>>("Q"),
    std::make_shared<OrderingBasedMinCutFuncBuilder<Hypergraph, maximum_adjacency_ordering>>("KW"),
    std::make_shared<ApproxMinCutBuilder<Hypergraph, approximate_minimizer<Hypergraph>>>("apxCX"),

    std::make_shared<CXMinCutBuilder<Hypergraph>>("CX"),
    std::make_shared<ApproxMinCutBuilder<Hypergraph, apxCertCX<MW_min_cut<Hypergraph>>>>("apxCertCX")
};


