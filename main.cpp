#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "hypergraph.h"
#include "fpz.h"
#include "cxy.h"

/* Attempts to read a hypergraph from a file */
bool parse_hypergraph(const std::string &filename, Hypergraph &hypergraph) {
  std::ifstream input;
  input.open(filename);

  int num_edges, num_vertices;
  input >> num_edges >> num_vertices;

  std::vector<std::vector<int>> edges;

  std::string line;
  std::getline(input, line); // Throw away first line

  while (std::getline(input, line)) {
    std::vector<int> edge;
    std::stringstream sstr(line);
    int i;
    while (sstr >> i) {
      edge.push_back(i);
    }
    edges.push_back(std::move(edge));
  }

  hypergraph = Hypergraph(num_vertices, edges);

  return input.eof();
}

int main(int argc, char *argv[]) {
  Hypergraph h;

  if (argc != 4) {
  usage:
    std::cerr << "Usage: " << argv[0]
              << " <input hypergraph filename> <k> <algorithm>\n"
              << "  algorithm: FPZ or CXY\n";
    return 1;
  }

  int k = std::atoi(argv[2]);

  if (!parse_hypergraph(argv[1], h)) {
    std::cout << "Failed to parse hypergraph in " << argv[1] << std::endl;
    return 1;
  }

  std::cout << "Read hypergraph with " << h.num_vertices() << " vertices and "
            << h.num_edges() << " edges" << std::endl;

  if (!h.is_valid()) {
    std::cout << "Hypergraph is not valid" << std::endl;
    std::cout << h << std::endl;
    return 1;
  }

  auto start = std::chrono::high_resolution_clock::now();
  int answer;
  if (argv[3] == std::string("FPZ")) {
    answer = fpz::branching_contract(h, k);
  } else if (argv[3] == std::string("CXY")) {
    answer = cxy::cxy_contract(h, k);
  } else {
    goto usage;
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "Algorithm took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                     start)
                   .count()
            << " milliseconds\n";
  std::cout << "The answer is " << answer << std::endl;
}
