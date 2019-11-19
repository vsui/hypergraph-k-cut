#include "hypergraph/hypergraph.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <numeric>
#include <cassert>

Hypergraph::Hypergraph() = default;

Hypergraph::Hypergraph(const Hypergraph &other) = default;

Hypergraph &Hypergraph::operator=(const Hypergraph &other) = default;

Hypergraph::Hypergraph(const std::vector<int> &vertices,
                       const std::vector<std::vector<int>> &edges) :
    next_vertex_id_(*std::max_element(std::begin(vertices), std::end(vertices)) + 1),
    next_edge_id_(static_cast<int>(edges.size())) {
  assert(vertices.size() > 0);
  for (const int v : vertices) {
    vertices_[v] = {};
    vertices_within_[v] = {v};
  }
  int e_i = -1;
  for (const auto &incident_vertices : edges) {
    edges_[++e_i] = incident_vertices;
    for (const int u : incident_vertices) {
      vertices_[u].push_back(e_i);
    }
  }
}

// TODO needing to copy vertices_within_ may be concerning from a performance standpoint
Hypergraph::Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
                       std::unordered_map<int, std::vector<int>> &&edges,
                       const Hypergraph &old)
    : vertices_(vertices), edges_(edges), next_vertex_id_(old.next_vertex_id_),
      next_edge_id_(old.next_edge_id_), vertices_within_(old.vertices_within_) {}

size_t Hypergraph::num_vertices() const { return vertices_.size(); }

size_t Hypergraph::num_edges() const { return edges_.size(); }

Hypergraph::vertex_range Hypergraph::vertices() const {
  return boost::adaptors::keys(vertices_);
}

const std::unordered_map<int, std::vector<int>> &Hypergraph::edges() const {
  return edges_;
}

bool Hypergraph::is_valid() const {
  for (const auto &[v, incidence] : vertices_) {
    for (const int e : incidence) {
      const auto &vertices_incident_on = edges_.at(e);
      if (std::find(vertices_incident_on.begin(), vertices_incident_on.end(),
                    v) == std::end(vertices_incident_on)) {
        std::cerr << "ERROR: edge " << e << " should contain vertex " << v
                  << std::endl;
        return false;
      }
    }
  }

  for (const auto &[e, incidence] : edges_) {
    for (const int v : incidence) {
      const auto &edges_incident_on = vertices_.at(v);
      if (std::find(edges_incident_on.begin(), edges_incident_on.end(), e) ==
          std::end(edges_incident_on)) {
        std::cerr << "ERROR: vertex " << v << " should contain edge " << e
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}


void Hypergraph::remove_hyperedge(int edge_id) {
  for (const auto v : edges_.at(edge_id)) {
    auto &vertex_incidence_list = vertices_.at(v);
    auto it = std::find(std::begin(vertex_incidence_list),
                        std::end(vertex_incidence_list), edge_id);
    // Swap it with the last element and pop it off to remove in O(1) time
    std::iter_swap(it, std::end(vertex_incidence_list) - 1);
    vertex_incidence_list.pop_back();
  }
  edges_.erase(edge_id);
}

const std::vector<int> &Hypergraph::edges_incident_on(int vertex_id) const {
  return vertices_.at(vertex_id);
}

std::istream &operator>>(std::istream &is, Hypergraph &hypergraph) {
  size_t num_edges, num_vertices;
  is >> num_edges >> num_vertices;

  std::vector<std::vector<int>> edges;

  std::string line;
  std::getline(is, line); // Throw away first line

  while (std::getline(is, line)) {
    std::vector<int> edge;
    std::stringstream sstr(line);
    int i;
    while (sstr >> i) {
      edge.push_back(i);
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

//std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph) {
//  os << hypergraph.num_edges() << " " << hypergraph.num_vertices() << std::endl;
//  os << "VERTICES\n";
//
//  for (const auto v : hypergraph.vertices()) {
//    os << v << ": ";
//    for (const int e : hypergraph.edges_incident_on(v)) {
//      os << e << " ";
//    }
//    os << std::endl;
//  }
//
//  os << "EDGES\n";
//  for (const auto &[id, vertices] : hypergraph.edges()) {
//    os << id << ": ";
//    for (const int v : vertices) {
//      os << v << " ";
//    }
//    os << std::endl;
//  }
//  return os;
//}

const std::list<int> &Hypergraph::vertices_within(const int v) const {
  return vertices_within_.at(v);
}

bool is_unweighted_hmetis_file(std::istream &is) {
  std::string line;
  std::getline(is, line);
  return std::count(std::begin(line), std::end(line), ' ') == 1;
}

