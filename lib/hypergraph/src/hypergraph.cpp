#include "hypergraph/hypergraph.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <numeric>
#include <cassert>

namespace hypergraphlib {

/**
 * This reads in a file in .hmetis format. Assumes that nodes are numbered from [0, n - 1], where n - 1 is the number of
 * vertices in the hypergraph.
 *
 * @param is
 * @param hypergraph
 * @return
 */
std::istream &operator>>(std::istream &is, Hypergraph &hypergraph) {
  size_t num_edges, num_vertices;
  is >> num_edges >> num_vertices;

  std::vector<std::vector<int>> edges;

  std::string line;
  std::getline(is, line); // Throw away first line

  int i = 0;
  while (i++ < num_edges && std::getline(is, line)) {
    std::vector<int> edge;
    std::stringstream sstr(line);
    int node;
    while (sstr >> node) {
      edge.push_back(node);
    }
    edges.push_back(std::move(edge));
  }

  std::vector<int> vertices(num_vertices);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  hypergraph = Hypergraph(vertices, edges);
  return is;
}

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph) {
  os << hypergraph.num_edges() << " " << hypergraph.num_vertices() << std::endl;

  for (const auto &[id, vertices] : hypergraph.edges()) {
    for (const int v : vertices) {
      os << v << " ";
    }
    os << std::endl;
  }
  return os;
}

bool is_unweighted_hmetis_file(std::istream &is) {
  std::string line;
  std::getline(is, line);
  return std::count(std::begin(line), std::end(line), ' ') == 1;
}

}
