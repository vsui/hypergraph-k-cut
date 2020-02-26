//
// Created by Victor on 2/25/20.
//

#include "store.hpp"
#include <iostream>

using std::begin, std::end;

int main() {
  FilesystemStore store("store3");
  for (CutInfo info : store.cuts()) {
    // Do nothing
    if (info.cut_value > 0 && std::none_of(begin(info.partitions),
                                           end(info.partitions),
                                           [](const auto &part) { return part.size() < 3; })) {
      std::cout << info.hypergraph << "," << info.cut_value << "," << info.partitions[0].size() << "," <<
                info.partitions[1].size() << std::endl;
    }
  }
  std::cout << "Done" << std::endl;
  return 0;
}