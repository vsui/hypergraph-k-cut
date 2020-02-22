// Search for hypergraphs with interesting (not-skewed) minimum cuts from generated hypergraph families

#include <cstddef>
#include <random>

#include <hypergraph/hypergraph.hpp>
#include <hypergraph/order.hpp>

using std::begin, std::end;

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

struct RandomRingHypergraph {
  size_t num_vertices;
  size_t num_hyperedges;
  size_t hyperedge_radius;

  RandomRingHypergraph(size_t n, size_t m, size_t r) : num_vertices(n), num_hyperedges(m), hyperedge_radius(r) {}

  Hypergraph generate();
};

struct RandomClusterHypergraph {
  size_t num_vertices;
  size_t num_clusters;
  size_t num_hyperedges_per_cluster;
  size_t num_hyperedges_spanning_clusters;
  size_t rank;

  RandomClusterHypergraph(size_t n, size_t c, size_t m1, size_t m2, size_t r) :
      num_vertices(n), num_clusters(c), num_hyperedges_per_cluster(m1), num_hyperedges_spanning_clusters(m2), rank(r) {}

  Hypergraph generate();
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

Hypergraph RandomRingHypergraph::generate() {
  std::random_device rd; // Get seed
  std::mt19937_64 gen(rd());
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

Hypergraph RandomClusterHypergraph::generate() {
  std::vector<int> vertices(num_vertices);
  std::iota(begin(vertices), end(vertices), 0);

  std::vector<std::vector<int>> edges;

  for (int i = 0; i < num_clusters; ++i) {
    Cluster cluster(num_vertices, num_clusters, i);
    std::vector<int> inside_cluster;
    for (auto it = cluster.begin(); it != cluster.end(); ++it) {
      inside_cluster.push_back(*it);
    }

    // Sample r vertices from this
    for (int j = 0; j < num_hyperedges_per_cluster; ++j) {
      std::vector<int> edge;
      std::sample(begin(inside_cluster),
                  end(inside_cluster),
                  std::back_inserter(edge),
                  rank,
                  std::mt19937_64(std::random_device{}()));
      edges.emplace_back(std::move(edge));
    }
  }

  // Sample r vertices from all clusters
  for (int j = 0; j < num_hyperedges_spanning_clusters; ++j) {
    std::vector<int> edge;
    std::sample(begin(vertices),
                end(vertices),
                std::back_inserter(edge),
                rank,
                std::mt19937_64(std::random_device{}()));
    edges.emplace_back(std::move(edge));
  }

  return Hypergraph{vertices, edges};
}

using RingHypergraphParams = std::tuple<size_t /* vertices */, size_t /* edges */, size_t /* radius */>;
using RandomClusterRankParams = std::tuple<size_t, size_t, size_t, size_t, size_t>;

void experiment2(const RandomClusterRankParams &param) {
  Hypergraph h = std::make_from_tuple<RandomClusterHypergraph>(param).generate();

  auto cut = MW_min_cut(h);

  if (cut.partitions[0].size() != 1 && cut.partitions[0].size() != h.num_vertices() - 1) {
    auto[n, clusters, m1, m2, r] = param;
    std::cout << n << " " << clusters << " " << m1 << " " << m2 << " " << r << std::endl;
    std::cout << cut << "---" << std::endl;
  }
}

void experiment(const RingHypergraphParams &param) {
  Hypergraph h = std::make_from_tuple<RandomRingHypergraph>(param).generate();

  auto cut = MW_min_cut(h);

  if (cut.partitions[0].size() != 1 && cut.partitions[0].size() != h.num_vertices() - 1) {
    auto[n, m, r] = param;
    std::cout << n << " " << m << " " << r << std::endl;
    std::cout << cut << "---" << std::endl;
  }
}

class Test {
  Test(int i, int x) {}
};

int main() {
  // std::make_from_tuple
  /*
  std::vector<RingHypergraphParams> params = {
      {100, 100, 10},
      {100, 100, 20},
      {100, 100, 30},
      {100, 100, 40},
      {100, 100, 50},
      {100, 100, 60},
      {100, 100, 70},
      {100, 100, 80},
      {100, 100, 90},
      {100, 1000, 10},
      {100, 1000, 20},
      {100, 1000, 30},
      {100, 1000, 40},
      {100, 1000, 50},
      {100, 1000, 60},
      {100, 1000, 70},
      {100, 1000, 80},
      {100, 1000, 90},
      {100, 10000, 10},
      {100, 10000, 20},
      {100, 10000, 30},
      {100, 10000, 40},
      {100, 10000, 50},
      {100, 10000, 60},
      {100, 10000, 70},
      {100, 10000, 80},
      {100, 10000, 90},
  };
   */

  std::vector<RandomClusterRankParams> params = {
      {100, 10, 100, 100, 3},
      {100, 10, 100, 100, 6},
      {100, 10, 100, 100, 9},
      {1000, 100, 1000, 1000, 3},
      {1000, 100, 1000, 1000, 6},
      {1000, 100, 1000, 1000, 9},
      {1000, 100, 1000, 1000, 30},
      {1000, 100, 1000, 1000, 60},
      {1000, 100, 1000, 1000, 90},
  };

  std::for_each(std::begin(params), std::end(params), experiment2);
}