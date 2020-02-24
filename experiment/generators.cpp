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

RandomRingHypergraph::RandomRingHypergraph(size_t n, size_t m, size_t r, uint64_t seed)
    : num_vertices(n), num_hyperedges(m), hyperedge_radius(r), seed(seed) {}

std::string RandomRingHypergraph::name() {
  std::stringstream stream;
  stream << "ring_" << num_vertices << "_" << num_hyperedges << "_" << hyperedge_radius << "_" << seed;
  return stream.str();
}

Hypergraph RandomRingHypergraph::generate() {
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution<> dis(0.0, 360.0);

  std::vector<int> vertices(num_vertices);
  std::iota(begin(vertices), end(vertices), 0);

  std::vector<double> vertex_positions(num_vertices);
  std::transform(begin(vertex_positions),
                 end(vertex_positions),
                 begin(vertex_positions),
                 [&dis, &gen](auto &a) { return dis(gen); });
  std::sort(begin(vertex_positions), end(vertex_positions)); // Sort for binary search access

  auto sample_hyperedge =
      [this, &vertices, &vertex_positions, &dis, &gen](std::vector<int> &edge) {
        double angle1 = dis(gen);
        double angle2 = angle1 + hyperedge_radius;

        for (int v : vertices) {
          if (angle_between(vertex_positions[v], angle1, angle2)) {
            edge.push_back(v);
          }
        }
      };

  std::vector<std::vector<int>> edges(num_hyperedges);
  std::for_each(begin(edges), end(edges), sample_hyperedge);

  return Hypergraph{vertices, edges};
}

