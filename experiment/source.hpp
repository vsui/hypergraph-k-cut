//
// Created by Victor on 2/23/20.
//

#ifndef HYPERGRAPHPARTITIONING_EXPERIMENT_SOURCE_HPP
#define HYPERGRAPHPARTITIONING_EXPERIMENT_SOURCE_HPP

#include "common.hpp"

class HypergraphSource {
public:
  virtual bool has_next() = 0;
  virtual HypergraphWrapper generate() = 0;
  virtual ~HypergraphSource() = default;
};

class PlantedHypergraphSource {
public:
  virtual bool has_next() = 0;
  virtual std::tuple<HypergraphWrapper, CutInfo> generate() = 0;
  virtual ~PlantedHypergraphSource() = default;
};

class AggregateSource : public HypergraphSource {
public:
  AggregateSource(std::vector<std::unique_ptr<HypergraphSource>> &&sources);
  bool has_next() override;
  HypergraphWrapper generate() override;
private:
  std::vector<std::unique_ptr<HypergraphSource>> sources_;
  decltype(sources_.begin()) it_;
};

template<bool Planted>
struct PlantedTrait;

template<>
struct PlantedTrait<true> {
  using GenT = std::tuple<HypergraphWrapper, CutInfo>;
};

template<>
struct PlantedTrait<false> {
  using GenT = HypergraphWrapper;
};

template<typename Generator, bool Planted, typename Interface>
class GenSource : public Interface {
  using GenT = typename PlantedTrait<Planted>::GenT;
  using Args = std::vector<typename Generator::ConstructorArgs>;

public:
  explicit GenSource(Args args) : args_(args), it_(args_.begin()) {}

  bool has_next() override { return it_ != args_.end(); }

  GenT generate() override {
    auto gen = std::make_from_tuple<Generator>(*it_);

    HypergraphWrapper h;

    h.name = gen.name();

    it_++;

    if constexpr (Planted) {
      auto g = gen.generate();
      h.h = std::get<0>(g);
      return {h, std::get<1>(g)};

    } else {
      h.h = gen.generate();
      return h;
    }

  }

private:
  Args args_;
  decltype(args_.begin()) it_;
};

template<typename Generator>
using GeneratorSource = GenSource<Generator, false, HypergraphSource>;

template<typename PlantedGenerator>
using PlantedGeneratorSource = GenSource<PlantedGenerator, true, PlantedHypergraphSource>;

/**
 * Takes the KCores of another generator
 */
class KCoreSource : public HypergraphSource {
public:
  explicit KCoreSource(std::unique_ptr<HypergraphSource> &&src);

  bool has_next() override;

  HypergraphWrapper generate() override;

private:
  std::unique_ptr<HypergraphSource> src_;
  size_t k = 2;
  HypergraphWrapper hypergraph_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_SOURCE_HPP
