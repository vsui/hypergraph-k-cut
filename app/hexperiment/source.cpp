//
// Created by Victor on 2/23/20.
//

#include "source.hpp"

AggregateSource::AggregateSource(std::vector<std::unique_ptr<HypergraphSource>> &&sources) :
    sources_(std::move(sources)), it_(sources_.begin()) {}

bool AggregateSource::has_next() {
  return it_ != sources_.end() || (*it_)->has_next();
}

HypergraphWrapper AggregateSource::generate() {
  if ((*it_)->has_next()) {
    return (*it_)->generate();
  }
  it_++;
  return generate();
}

KCoreSource::KCoreSource(std::unique_ptr<HypergraphSource> &&src)
    : src_(std::move(src)), hypergraph_(src_->generate()) {}

bool KCoreSource::has_next() {
  std::cout << hypergraph_.name << " " << k << std::endl;
  return src_->has_next() || k < 6;
}

HypergraphWrapper KCoreSource::generate() {
  std::cout << hypergraph_.name << " " << k << std::endl;
  if (k == 6 && src_->has_next()) {
    hypergraph_ = src_->generate();
    k = 2;
  }

  HypergraphWrapper ret;
  ret.name = hypergraph_.name + "_" + std::to_string(k) + "core";

  Hypergraph hgr = std::get<Hypergraph>(hypergraph_.h);

  ret.h = kCoreDecomposition(hgr, k);

  ++k;

  return ret;
}
