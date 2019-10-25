#include "hypergraph.h"

#include <iostream>
#include <sstream>
#include <vector>

Hypergraph::Hypergraph() = default;
Hypergraph::Hypergraph(const Hypergraph &other) = default;
Hypergraph &Hypergraph::operator=(const Hypergraph &other) = default;

Hypergraph::Hypergraph(const int num_edges,
                       const std::vector<std::vector<int>> &edges)
    : next_vertex_id_(num_edges) {
  for (int i = 0; i < num_edges; ++i) {
    vertices_[i] = {};
  }

  int next_edge_id = 0;
  for (const auto &incident_on : edges) {
    for (const int v : incident_on) {
      vertices_[v].push_back(next_edge_id);
      edges_[next_edge_id].push_back(v);
    }
    ++next_edge_id;
  }
}

Hypergraph::Hypergraph(const std::vector<int> &vertices,
                       const std::vector<std::vector<int>> &edges) {
  for (const int v : vertices) {
    vertices_[v] = {};
  }
  next_vertex_id_ = *std::max(std::begin(vertices), std::end(vertices));
  int e_i = -1;
  for (const auto &incident_vertices : edges) {
    edges_[++e_i] = incident_vertices;
    for (const int u : incident_vertices) {
      vertices_[u].push_back(e_i);
    }
  }
}

Hypergraph::Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
                       std::unordered_map<int, std::vector<int>> &&edges,
                       int next_vertex_id)
    : vertices_(vertices), edges_(edges), next_vertex_id_(next_vertex_id) {}

size_t Hypergraph::num_vertices() const { return vertices_.size(); }
size_t Hypergraph::num_edges() const { return edges_.size(); }

std::unordered_map<int, std::vector<int>> &Hypergraph::vertices() {
  return vertices_;
}

std::unordered_map<int, std::vector<int>> &Hypergraph::edges() {
  return edges_;
}

const std::unordered_map<int, std::vector<int>> &Hypergraph::vertices() const {
  return vertices_;
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

/* Returns a new hypergraph with the edge contracted. Assumes that there is
 * such an edge in the hypergraph */
Hypergraph Hypergraph::contract(const int edge_id) const {
  // Set V' := V \ e (do not copy incidence lists)
  std::unordered_map<int, std::vector<int>> new_vertices;
  new_vertices.reserve(vertices_.size());
  for (const auto &[id, incidence] : vertices_) {
    new_vertices[id] = {};
  }
  for (const int id : edges_.at(edge_id)) {
    new_vertices.erase(id);
  }

  // Set E' := E \ { e } (do not copy incidence lists)
  std::unordered_map<int, std::vector<int>> new_edges;
  new_edges.reserve(edges_.size());
  for (const auto &[id, incidence] : edges_) {
    new_edges[id] = {};
  }
  new_edges.erase(edge_id);

  // for v' in V'
  for (auto &[v, v_edges] : new_vertices) {
    // for e' in E' incident to v in H
    for (const int e : vertices_.at(v)) {
      auto &e_vertices = new_edges.at(e);
      // add v' to the incidence list of e'
      e_vertices.push_back(v);
      // add e' to the incidence list of v'
      v_edges.push_back(e);
    }
  }

  // Remove any e' with empty incidence list (it was a subset of the removed
  // hyperedge)
  for (auto it = std::begin(new_edges); it != std::end(new_edges);) {
    if (it->second.empty()) {
      it = new_edges.erase(it);
    } else {
      ++it;
    }
  }

  // Now add a new vertex v_e replacing the removed hyperedge
  int v_e = next_vertex_id_++;
  new_vertices[v_e] = {};

  // Add v_e to all hyperedges with less edges and vice versa
  for (auto &[e, new_incident_on] : new_edges) {
    const auto &old_incident_on = edges_.at(e);

    if (new_incident_on.size() < old_incident_on.size()) {
      new_incident_on.push_back(v_e);
      new_vertices[v_e].push_back(e);
    }
  }

  Hypergraph new_hypergraph(std::move(new_vertices), std::move(new_edges),
                            next_vertex_id_);

  // May be useful for debugging later
  /*
  if (!new_hypergraph.is_valid()) {
    std::cerr << "Contraction of edge " << edge_id
              << " created invalid hypergraph\n";

    std::cout << *this;
    std::cout << "TO\n";
    std::cout << new_hypergraph;
    throw 444;
  }
  */

  return new_hypergraph;
}

std::istream &operator>>(std::istream &is, Hypergraph &hypergraph) {
  int num_edges, num_vertices;
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

  hypergraph = Hypergraph(num_vertices, edges);
  return is;
}

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph) {
  os << hypergraph.num_edges() << " " << hypergraph.num_vertices() << std::endl;
  os << "VERTICES\n";

  for (const auto &[id, edges] : hypergraph.vertices()) {
    os << id << ": ";
    for (const int e : edges) {
      os << e << " ";
    }
    os << std::endl;
  }

  os << "EDGES\n";
  for (const auto &[id, vertices] : hypergraph.edges()) {
    os << id << ": ";
    for (const int v : vertices) {
      os << v << " ";
    }
    os << std::endl;
  }
  return os;
}

