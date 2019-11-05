#include <fstream>
#include <iostream>
#include <vector>

#include "hypergraph/approx.hpp"
#include "hypergraph/cxy.hpp"
#include "hypergraph/fpz.hpp"
#include "hypergraph/hypergraph.hpp"
#include "hypergraph/order.hpp"

/* Attempts to read a hypergraph from a file */
bool parse_hypergraph(const std::string &filename, Hypergraph &hypergraph) {
  std::ifstream input;
  input.open(filename);

  input >> hypergraph;

  return input.eof();
}

int main(int argc, char *argv[]) {
  Hypergraph h;

  if (argc != 4 && argc != 5 /* for epsilon */) {
    usage:
    std::cerr << "Usage: " << argv[0]
              << " <input hypergraph filename> <k> <algorithm>\n"
              << "  algorithm: FPZ, CXY, Q, KW or MW\n";
    return 1;
  }

  size_t k = std::stoul(argv[2]);

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

  //  std::unordered_map<int, size_t> edge_weights;
  //  for (int e = 0; e < h.num_edges(); ++e) {
  //    edge_weights[e] = e + 1;
  //  }
  //  h.set_edge_weights(edge_weights);
  //
  auto start = std::chrono::high_resolution_clock::now();
  size_t answer;
  if (argv[3] == std::string("FPZ")) {
    answer = fpz::branching_contract(h, k, 0, true);
  } else if (argv[3] == std::string("CXY")) {
    answer = cxy::cxy_contract(h, k, 0, true);
  } else if (argv[3] == std::string("Q")) {
    if (k != 2) {
      std::cout << "Q can only compute mincuts" << std::endl;
      return 1;
    }
    answer = vertex_ordering_mincut<Hypergraph, queyranne_ordering>(h, 1);
  } else if (argv[3] == std::string("KW")) {
    if (k != 2) {
      std::cout << "KW can only compute mincuts" << std::endl;
      return 1;
    }
    answer = vertex_ordering_mincut<Hypergraph, maximum_adjacency_ordering>(h, 1);
  } else if (argv[3] == std::string("MW")) {
    if (k != 2) {
      std::cout << "MW can only compute mincuts" << std::endl;
      return 1;
    }
    answer = vertex_ordering_mincut<Hypergraph, tight_ordering>(h, 1);
  } else if (argv[3] == std::string("sparseQ")) {
    if (k != 2) {
      std::cout << "sparseQ can only compute mincuts" << std::endl;
      return 1;
    }
    answer = vertex_ordering_mincut_certificate<queyranne_ordering>(h, 1);
  } else if (argv[3] == std::string("sparseKW")) {
    if (k != 2) {
      std::cout << "sparseKW can only compute mincuts" << std::endl;
      return 1;
    }
    answer =
        vertex_ordering_mincut_certificate<maximum_adjacency_ordering>(h, 1);
  } else if (argv[3] == std::string("sparseMW")) {
    if (k != 2) {
      std::cout << "sparseMW can only compute mincuts" << std::endl;
      return 1;
    }
    answer = vertex_ordering_mincut_certificate<tight_ordering>(h, 1);
  } else if (argv[3] == std::string("CXapprox")) {
    if (k != 2) {
      std::cout << "CXapprox can only compute mincuts" << std::endl;
      return 1;
    }
    double epsilon = std::strtod(argv[4], nullptr);
    answer = approximate_minimizer(h, epsilon);
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
