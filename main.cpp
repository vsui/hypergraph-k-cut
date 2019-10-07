#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

class Hypergraph;
std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph);

class Hypergraph {
public:
  Hypergraph &operator=(const Hypergraph &other) = default;

  Hypergraph() = default;
  Hypergraph(const Hypergraph &other) = default;

  Hypergraph(const int num_edges, const std::vector<std::vector<int>> &edges)
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

  /* Constructor that directly sets adjacency lists and next vertex id */
  Hypergraph(std::unordered_map<int, std::vector<int>> &&vertices,
             std::unordered_map<int, std::vector<int>> &&edges,
             int next_vertex_id)
      : vertices_(vertices), edges_(edges), next_vertex_id_(next_vertex_id) {}

  int num_vertices() const { return vertices_.size(); }
  int num_edges() const { return edges_.size(); }

  std::unordered_map<int, std::vector<int>> &vertices() { return vertices_; }

  std::unordered_map<int, std::vector<int>> &edges() { return edges_; }

  const std::unordered_map<int, std::vector<int>> &vertices() const {
    return vertices_;
  }

  const std::unordered_map<int, std::vector<int>> &edges() const {
    return edges_;
  }

  /* Checks that the internal state of the hypergraph is consistent. Mainly for
   * debugging */
  bool is_valid() const {
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
  Hypergraph contract(const int edge_id) const {
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

private:
  mutable int next_vertex_id_;

  // Map of vertex IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> vertices_;

  // Map of edge IDs -> incidence lists
  std::unordered_map<int, std::vector<int>> edges_;
};

std::ostream &operator<<(std::ostream &os, const Hypergraph &hypergraph) {
  os << hypergraph.num_edges() << " " << hypergraph.num_vertices() << std::endl;
  os << "VERTICES\n";

  for (const auto &[id, edges] : hypergraph.vertices()) {
    std::cout << id << ": ";
    for (const int e : edges) {
      os << e << " ";
    }
    os << std::endl;
  }

  os << "EDGES\n";
  for (const auto &[id, vertices] : hypergraph.edges()) {
    std::cout << id << ": ";
    for (const int v : vertices) {
      os << v << " ";
    }
    os << std::endl;
  }
  return os;
}

/* Redo probability from the branching contraction paper. The calculations have
 * been changed a bit to make calculation efficient and to avoid overflow
 *
 * Args:
 *   n: number of vertices in the hypergraph
 *   e: number of vertices that the hyperedge is incident on
 *   k: number of partitions
 */
double redo_probability(int n, int e, int k) {
  double s = 0;
  for (int i = n - e - k + 2; i <= n - e; ++i) {
    s += std::log(i);
  }
  for (int i = n - k + 2; i <= n; ++i) {
    s -= std::log(i);
  }
  return 1 - std::exp(s);
}

/* Branching contraction min cut inner routine
 *
 * accumulated : a running count of k-spanning hyperedges used to calculate the
 *               min cut
 * */
int branching_contract_(Hypergraph &hypergraph, int k, int accumulated = 0) {
  // Remove k-spanning hyperedges from hypergraph
  for (auto it = std::begin(hypergraph.edges());
       it != std::end(hypergraph.edges());) {
    if (it->second.size() >= hypergraph.num_vertices() - k + 2) {
      ++accumulated;
      // Remove k-spanning hyperedge from incidence lists
      for (const int v : it->second) {
        auto &vertex_incidence_list = hypergraph.vertices().at(v);
        auto it2 = std::find(std::begin(vertex_incidence_list),
                             std::end(vertex_incidence_list), it->first);
        // Swap it with the last element and pop it off to remove in O(1) time
        std::iter_swap(it2, std::end(vertex_incidence_list) - 1);
        vertex_incidence_list.pop_back();
      }
      // Remove k-spanning hyperedge
      it = hypergraph.edges().erase(it);
    } else {
      ++it;
    }
  }

  // If no edges remain, return the answer
  if (hypergraph.num_edges() == 0) {
    return accumulated;
  }

  // Static random device for random sampling and generating random numbers
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> dis(0.0, 1.0);

  // Select a hyperedge with probability proportional to its weight
  // Note: dealing with unweighted hyperedges
  std::pair<int, std::vector<int>> sampled[1];
  std::sample(std::begin(hypergraph.edges()), std::end(hypergraph.edges()),
              std::begin(sampled), 1, gen);

  double redo =
      redo_probability(hypergraph.num_vertices(), sampled[0].second.size(), k);

  Hypergraph contracted = hypergraph.contract(sampled->first);

  if (dis(gen) < redo) {
    return std::min(branching_contract_(contracted, k, accumulated),
                    branching_contract_(hypergraph, k, accumulated));
  } else {
    return branching_contract_(contracted, k, accumulated);
  }
}

/* Runs branching contraction log^2(n) times and returns the minimum */
int branching_contract(Hypergraph &hypergraph, int k) {
  int logn = std::log(hypergraph.num_vertices());
  int repeat = logn * logn;

  int min_so_far = std::numeric_limits<int>::max();
  for (int i = 0; i < repeat; ++i) {
    // branching_contract_ modifies the input hypergraph so avoid copying, so
    // copy it once in the beginning to save time
    Hypergraph copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    min_so_far = std::min(min_so_far, branching_contract_(copy, k));
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[" << i+1 << "/" << repeat << "] took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " milliseconds\n";
  }

  return min_so_far;
}

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

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <input hypergraph filename> <k>"
              << std::endl;
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
  int answer = branching_contract(h, k);
  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "Algorithm took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                     start)
                   .count()
            << " milliseconds\n";
  std::cout << "The answer is " << answer << std::endl;
}
