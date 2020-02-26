//
// Created by Victor on 2/25/20.
//

#include "store.hpp"
#include <iostream>

int main() {
  std::cout << "1" << std::endl;
  FilesystemStore store("store");
  std::cout << "2" << std::endl;

  for (CutInfo info : store.cuts()) {
    // Do nothing
    std::cout << info << std::endl;
  }
  std::cout << "Done" << std::endl;
  return 0;
}