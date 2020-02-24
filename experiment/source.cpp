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
