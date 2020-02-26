//
// Created by Victor on 2/23/20.
//

#include "generators.hpp"

namespace {

// Helper for logic with cluster operations
class Cluster {
public:
  Cluster(size_t n, size_t k, size_t ki) : n_(n), k_(k), ki_(ki) {}

  struct ClusterIt {
    ClusterIt(size_t v) : v(v) {}
    ClusterIt &operator++() {
      v++;
      return *this;
    }
    size_t operator*() { return v; }
    bool operator!=(const ClusterIt &other) { return other.v != v; }

    size_t v;
  };

  ClusterIt begin() { return ClusterIt(vertices_before()); }
  ClusterIt end() { return ClusterIt(vertices_before() + size()); }
  Cluster operator*() { return *this; }
  Cluster &operator++() {
    ki_++;
    return *this;
  }
  bool operator!=(const Cluster &other) {
    return other.n_ != n_ || other.k_ != k_ || other.ki_ != ki_;
  }

private:
  size_t vertices_before() { return size_() * ki_; }
  size_t size_() { return n_ / k_; }
  size_t size() { return size_() + (ki_ == k_ - 1 ? (n_ % k_) : 0); }

  size_t n_;
  size_t k_;
  size_t ki_;
};

/**
 * Determine whether `angle` lies in `[a,b]`. All parameters are in `[0,360]`, except b, which is in `[0,720]`.
 */
bool angle_between(double angle, double a, double b) {
  if (b > 360) {
    return angle_between(angle, a, 360) || angle_between(angle, 0, b - 360);
  }
  return a <= angle && angle <= b;
}

}

RandomRingHypergraph::RandomRingHypergraph(size_t num_vertices,
                                           size_t num_hyperedges,
                                           double hyperedge_mean,
                                           double hyperedge_variance,
                                           uint64_t seed) :
    num_vertices(num_vertices), num_hyperedges(num_hyperedges), hyperedge_mean(hyperedge_mean),
    hyperedge_variance(hyperedge_variance), seed(seed), gen_(seed), dis_(0.0, 360.0) {}

Hypergraph RandomRingHypergraph::generate() {
  gen_.seed(seed); // We want to generate the same hypergraph each time

  std::vector<int> vertices(num_vertices);
  std::iota(begin(vertices), end(vertices), 0);

  std::vector<double> vertex_positions(num_vertices);
  std::transform(begin(vertex_positions),
                 end(vertex_positions),
                 begin(vertex_positions),
                 [this](auto &a) { return dis_(gen_); });
  std::sort(begin(vertex_positions), end(vertex_positions)); // Sort for binary search access

  auto sample =
      [this, &vertices, &vertex_positions](std::vector<int> &edge) {
        sample_hyperedge(vertices, vertex_positions, edge);
      };

  std::vector<std::vector<int>> edges(num_hyperedges);
  std::for_each(begin(edges), end(edges), sample);

  return Hypergraph{vertices, edges};
}

void RandomRingHypergraph::sample_hyperedge(const std::vector<int> &vertices,
                                            const std::vector<double> &positions,
                                            std::vector<int> &edge) {
  std::normal_distribution<> normal_dis{hyperedge_mean, hyperedge_variance};

  double angle1 = dis_(gen_);
  double angle2 = angle1 + normal_dis(gen_);

  for (int v : vertices) {
    if (angle_between(positions[v], angle1, angle2)) {
      edge.push_back(v);
    }
  }
}

RandomRingConstantEdgeHypergraph::RandomRingConstantEdgeHypergraph(size_t n, size_t m, double r, uint64_t seed) :
    RandomRingHypergraph(n, m, r, 0, seed) {}

std::string RandomRingConstantEdgeHypergraph::name() {
  std::stringstream stream;
  stream << "constantring_" << num_vertices << "_" << num_hyperedges << "_" << hyperedge_mean << "_" << seed;
  return stream.str();
}

std::string RandomRingVariableEdgeHypergraph::name() {
  std::stringstream stream;
  stream << "variablering_" << num_vertices << "_" << num_hyperedges << "_" << hyperedge_mean << "_"
         << hyperedge_variance << "_" << seed;
  return stream.str();
}

MXNHypergraph::MXNHypergraph(size_t n, size_t m, double p, uint64_t seed) : n(n), m(m), p(p), seed(seed) {}

Hypergraph MXNHypergraph::generate() {
  std::mt19937_64 rand(seed);

  std::uniform_real_distribution<> distribution(0, 1);

  std::vector<std::vector<int>> edges;
  for (int i = 0; i < m; ++i) {
    std::vector<int> edge;
    for (int v = 0; v < n; ++v) {
      if (distribution(rand) < p) {
        edge.push_back(v);
      }
    }
    edges.emplace_back(std::move(edge));
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  return Hypergraph{vertices, edges};
}

std::string MXNHypergraph::name() {
  std::stringstream ss;
  ss << "mxn_" << m << "_" << n << "_" << p << "_" << seed;
  return ss.str();
}
