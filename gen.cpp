#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <hypergraph/hypergraph.hpp>

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

  int input;
  std::cout << "Enter \"1\" for an unweighted graph, \"2\" for a weighted graph" << std::endl;
  std::cin >> input;
  if (input != 1 && input != 2) {
    std::cerr << "Bad input" << std::endl;
    return 1;
  }

  bool unweighted = 1 == input;

  std::vector<std::vector<int>> edges;
  int i = 0;
  while (i < n_edges) {
    std::vector<int> edge;
    while (edge.size() == 0) {
      edge = {};
      for (int v = 0; v < n_vertices; ++v) {
        if (distribution(e2) < sampling_probability / 100.0) {
          edge.push_back(v);
        }
      }
    }
    if (edge.size() < 2) {
      continue;
    }
    edges.push_back(std::move(edge));
    ++i;
  }

  std::ofstream output;
  output.open(filename);

  std::vector<int> vertices(n_vertices);
  std::iota(std::begin(vertices), std::end(vertices), 0);
  if (unweighted) {
    Hypergraph hypergraph(vertices, edges);
    output << hypergraph;
  } else {
    std::vector<std::pair<std::vector<int>, double>> weighted_edges;
    std::transform(std::begin(edges),
                   std::end(edges),
                   std::back_inserter(weighted_edges),
                   [&distribution, &e2](const auto &edge) {
                     return std::make_pair(edge, distribution(e2));
                   });
    WeightedHypergraph<double> hypergraph(vertices, weighted_edges);
    output << hypergraph;
  }

  output.close();
  std::cout << filename;
}
