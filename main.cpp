#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "cxy.h"
#include "fpz.h"
#include "hypergraph.h"
#include "order.h"

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
              << "  algorithm: FPZ, CXY, Q, KW or MW\n";
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
  }  else if (argv[3] == std::string("Q")) {
    if (k != 2) { std::cout << "Q can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut(h, 1, queyranne_ordering);
  } else if (argv[3] == std::string("KW")) {
    if (k != 2) { std::cout << "KW can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut(h, 1, maximum_adjacency_ordering);
  } else if (argv[3] == std::string("MW")) {
    if (k != 2) { std::cout << "MW can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut(h, 1, tight_ordering);
  }  else if (argv[3] == std::string("sparseQ")) {
    if (k != 2) { std::cout << "sparseQ can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut_certificate(h, 1, queyranne_ordering);
  } else if (argv[3] == std::string("sparseKW")) {
    if (k != 2) { std::cout << "sparseKW can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut_certificate(h, 1, maximum_adjacency_ordering);
  } else if (argv[3] == std::string("sparseMW")) {
    if (k != 2) { std::cout << "sparseMW can only compute mincuts" << std::endl; return 1; }
    answer = vertex_ordering_mincut_certificate(h, 1, tight_ordering);
  }
  else {
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
