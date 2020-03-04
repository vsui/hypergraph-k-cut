//
// Created by Victor on 2/23/20.
//

#include "generators.hpp"

#include <sqlite3.h>

using std::begin, std::end;

namespace {

// Helper for logic with cluster operations
class Cluster {
public:
  Cluster(size_t n, size_t k, size_t ki) : n_(n), k_(k), ki_(ki) {}

  struct ClusterIt {
    explicit ClusterIt(size_t v) : v(v) {}
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

  static Cluster get_cluster(const int v, size_t num_clusters, size_t num_vertices) {
    for (int i = 0; i < num_clusters; ++i) {
      Cluster cluster(num_vertices, num_clusters, i);
      auto it = std::find(std::begin(cluster), std::end(cluster), v);
      if (it != std::end(cluster)) {
        return cluster;
      }
    }
    // Should not be reachable
    assert(false);
  }

  static bool edge_crosses_clusters(const std::vector<int> &edge, size_t num_clusters, size_t num_vertices) {
    if (edge.size() < 2) {
      return false;
    }
    Cluster c = get_cluster(edge.at(0), num_clusters, num_vertices);
    return std::any_of(std::begin(edge), std::end(edge), [&c, num_clusters, num_vertices](const int v) {
      return Cluster::get_cluster(v, num_clusters, num_vertices) != c;
    });
  }

  static HypergraphCut<size_t> create_cut_from_cluster(const size_t cut_value,
                                                       const size_t num_clusters,
                                                       const size_t num_vertices) {
    HypergraphCut<size_t> cut = HypergraphCut<size_t>::max();
    for (int i = 0; i < num_clusters; ++i) {
      Cluster cluster(num_vertices, num_clusters, i);
      std::vector<int> p;
      for (auto v : cluster) {
        p.emplace_back(v);
      }
      cut.partitions.emplace_back(std::move(p));
    }
    return cut;
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

int null_callback(void *,
                  int,
                  char **,
                  char **) {
  return 0;
}

}

template<>
struct std::iterator_traits<Cluster::ClusterIt> {
  using iterator_category = std::input_iterator_tag;
};

RandomRingHypergraph::RandomRingHypergraph(size_t num_vertices,
                                           size_t num_hyperedges,
                                           double hyperedge_mean,
                                           double hyperedge_variance,
                                           uint64_t seed) :
    num_vertices(num_vertices), num_hyperedges(num_hyperedges), hyperedge_mean(hyperedge_mean),
    hyperedge_variance(hyperedge_variance), seed(seed), gen_(seed), dis_(0.0, 360.0) {}

HypergraphGenerator::~HypergraphGenerator() = default;

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

PlantedHypergraph::PlantedHypergraph(size_t n, size_t m1, double p1, size_t m2, double p2, size_t k, uint64_t seed) :
    n(n), m1(m1), p1(p1), m2(m2), p2(p2), k(k), seed(seed) {}

std::tuple<Hypergraph, std::optional<CutInfo>> PlantedHypergraph::generate() const {
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution dis;

  std::vector<int> vertices(n, 0);
  std::iota(begin(vertices), end(vertices), 0);
  std::vector<std::vector<int>> edges;

  // Sample m1 hyperedges from each cluster, each vertex has p1 probability of being sampled
  for (int i = 0; i < k; ++i) {
    Cluster cluster(n, k, i);
    for (int j = 0; j < m1; ++j) {
      std::vector<int> edge;
      for (const auto v : cluster) {
        if (dis(gen) < p1) {
          edge.push_back(v);
        }
      }
      edges.emplace_back(std::move(edge));
    }
  }

  size_t cut_value = 0;

  // Sample m2 hyperedges from the entire hypergraph, each vertex has p2 probability of being sampled
  for (int j = 0; j < m2; ++j) {
    std::vector<int> edge;
    for (const auto v : vertices) {
      if (dis(gen) < p2) {
        edge.push_back(v);
      }
    }
    if (Cluster::edge_crosses_clusters(edge, k, n)) {
      ++cut_value;
    }
    edges.emplace_back(std::move(edge));
  }

  HypergraphCut<size_t> cut = Cluster::create_cut_from_cluster(cut_value, k, n);

  return {{Hypergraph{vertices, edges}}, {{k, cut}}};
}

std::string PlantedHypergraph::name() const {
  std::stringstream ss;
  ss << "planted"
     << "_" << n
     << "_" << m1
     << "_" << p1
     << "_" << m2
     << "_" << p2
     << "_" << k
     << "_" << seed;
  return ss.str();
}

std::string PlantedHypergraph::make_table_sql_command() {
  return R"(
CREATE TABLE IF NOT EXISTS planted_hypergraphs (
  id TEXT PRIMARY KEY NOT NULL,
  n INTEGER NOT NULL,
  m1 INTEGER NOT NULL,
  p1 REAL NOT NULL,
  m2 INTEGER NOT NULL,
  p2 REAL NOT NULL,
  k INTEGER NOT NULL,
  seed INTEGER NOT NULL,
  FOREIGN KEY (id)
    REFERENCES hypergraphs (id)
);)";
}

bool PlantedHypergraph::write_to_table(sqlite3 *db) const {
  std::stringstream command;
  command << "INSERT INTO planted_hypergraphs (id, n, m1, p1, m2, p2, k, seed) VALUES "
          << "(" << "'" << name() << "'"
          << ", " << n
          << ", " << m1
          << ", " << p1
          << ", " << m2
          << ", " << p2
          << ", " << k
          << ", " << seed
          << ");";
  char *zErrMsg{};
  int err = sqlite3_exec(db, command.str().c_str(), null_callback, nullptr, &zErrMsg);
  if (err != SQLITE_OK) {
    if (std::string(zErrMsg) == "UNIQUE constraint failed: planted_hypergraphs.id") {
      return true;
    }
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    return false;
  }
  return true;
}

UniformPlantedHypergraph::UniformPlantedHypergraph(size_t n, size_t k, size_t r, size_t m1, size_t m2, uint64_t seed) :
    n(n), k(k), r(r), m1(m1), m2(m2), seed(seed) {}

std::tuple<Hypergraph, std::optional<CutInfo>> UniformPlantedHypergraph::generate() const {
  std::mt19937_64 gen(seed);
  std::vector<int> vertices(n);
  std::iota(begin(vertices), end(vertices), 0);

  std::vector<std::vector<int>> edges;

  for (int i = 0; i < k; ++i) {
    Cluster cluster(n, k, i);
    std::vector<int> inside_cluster;
    for (const auto v : cluster) {
      inside_cluster.push_back(v);
    }

    // Sample r vertices from this
    for (int j = 0; j < m1; ++j) {
      std::vector<int> edge;
      std::sample(begin(inside_cluster),
                  end(inside_cluster),
                  std::back_inserter(edge),
                  r,
                  gen);
      edges.emplace_back(std::move(edge));
    }
  }

  size_t cut_value = 0;
  // Sample r vertices from all clusters
  for (int j = 0; j < m2; ++j) {
    std::vector<int> edge;
    std::sample(begin(vertices),
                end(vertices),
                std::back_inserter(edge),
                r,
                gen);
    if (Cluster::edge_crosses_clusters(edge, k, n)) {
      ++cut_value;
    }
    edges.emplace_back(std::move(edge));
  }
  HypergraphCut<size_t> cut = Cluster::create_cut_from_cluster(cut_value, k, n);

  return {{Hypergraph{vertices, edges}}, {{k, cut}}};
}
std::string UniformPlantedHypergraph::name() const {
  std::stringstream s;
  s << "uniformplanted_" << n << "_" << k << "_" << r << "_" << m1 << "_" << m2;
  return s.str();
}
