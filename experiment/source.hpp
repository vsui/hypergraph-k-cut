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

class AggregateSource : public HypergraphSource {
public:
  AggregateSource(std::vector<std::unique_ptr<HypergraphSource>> &&sources);
  bool has_next() override;
  HypergraphWrapper generate() override;
private:
  std::vector<std::unique_ptr<HypergraphSource>> sources_;
  decltype(sources_.begin()) it_;
};

template<typename Generator>
class GeneratorSource : public HypergraphSource {
  using Args = std::vector<typename Generator::ConstructorArgs>;

public:
  explicit GeneratorSource(Args args) : args_(args), it_(args_.begin()) {}

  bool has_next() override { return it_ != args_.end(); }

  HypergraphWrapper generate() override {
    auto gen = std::make_from_tuple<Generator>(*it_);

    HypergraphWrapper h;

    h.name = gen.name();
    h.h = gen.generate();

    it_++;
    return h;
  }

private:
  Args args_;
  decltype(args_.begin()) it_;
};

#endif //HYPERGRAPHPARTITIONING_EXPERIMENT_SOURCE_HPP
