#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "hypergraph.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Generates a hypergraph file with a specified number of "
                 "vertices and edges. Each edge samples from each vertex with "
                 "the sampling probability.\n"
              << "Usage:\n"
              << argv[0]
              << " <# vertices> <# edges> <sampling probability (0-100)>\n";
    return 1;
  }

  int n_vertices = std::atoi(argv[1]);
  int n_edges = std::atoi(argv[2]);
  int sampling_probability = std::atoi(argv[3]);

  std::stringstream sstr;
  sstr << n_vertices << "_" << n_edges << "_" << sampling_probability << ".hgr";
  std::string filename = sstr.str();

  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);

  std::vector<std::vector<int>> edges;
  for (int i = 0; i < n_edges; ++i) {
    std::vector<int> edge;
    while (edge.size() == 0) {
      edge = {};
      for (int v = 0; v < n_vertices; ++v) {
        if (distribution(e2) < sampling_probability / 100.0) {
          edge.push_back(v);
        }
      }
    }
    edges.push_back(std::move(edge));
  }

  std::ofstream output;
  output.open(filename);

  if (!output.is_open()) {
    std::cerr << "Failed to open file" << std::endl;
    return 1;
  }

  // hMETIS hypergraph format
  output << n_edges << " " << n_vertices << std::endl;
  for (const auto &edge : edges) {
    output << edge[0];
    for (auto it = std::cbegin(edge) + 1; it != std::cend(edge); ++it) {
      output << " " << *it;
    }
    output << std::endl;
  }

  output.close();

  std::cout << filename;
}
