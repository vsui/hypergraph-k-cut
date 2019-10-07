#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "hypergraph.h"

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
    int answer_ = branching_contract_(copy, k);
    min_so_far = std::min(min_so_far, answer_);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[" << i + 1 << "/" << repeat << "] took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " milliseconds, got " << answer_ << "\n";
  }

  return min_so_far;
}

/* Calculates delta from CXY.
 *
 * Args:
 *   n: number of vertices
 *   e: the size of the hyperedge
 *   k: number of partitions
 */
double cxy_delta(int n, int e, int k) {
  int s = 0;
  for (int i = n - e - k + 2; i <= n - e; ++i) {
    s += std::log(i);
  }
  for (int i = n - k + 2; i <= n; ++i) {
    s -= std::log(i);
  }
  return std::exp(s);
}

int cxy_contract_(Hypergraph &hypergraph, int k) {
  std::vector<int> candidates = {};
  std::vector<int> edge_ids;
  std::vector<double> deltas;

  int min_so_far = hypergraph.edges().size();

  while (true) {
    if (hypergraph.num_vertices() <= 4 * k - 4) {
      // BF k-cut-sets in G
    }

    edge_ids.resize(hypergraph.edges().size());
    deltas.resize(hypergraph.edges().size());

    int i = 0;
    for (const auto &[edge_id, incidence] : hypergraph.edges()) {
      edge_ids[i] = edge_id;
      deltas[i] = cxy_delta(hypergraph.num_vertices(), incidence.size(), k);
      ++i;
    }

    if (std::accumulate(std::begin(deltas), std::end(deltas), 0.0) == 0) {
      min_so_far = std::min(min_so_far, hypergraph.num_edges());
      break;
    }

    static std::default_random_engine generator;
    std::discrete_distribution<int> distribution(std::begin(deltas),
                                                 std::end(deltas));

    int sampled = distribution(generator);
    int sampled_id = edge_ids.at(sampled);

    hypergraph = hypergraph.contract(sampled_id);
  }

  return min_so_far;
}

/* n choose r
 *
 * [https://stackoverflow.com/questions/9330915/number-of-combinations-n-choose-r-in-c]
 */
long long ncr(int n, int k) {
  if (k > n) {
    return 0;
  }
  if (k * 2 > n) {
    k = n - k;
  }
  if (k == 0) {
    return 1;
  }

  long long result = n;
  for (int i = 2; i <= k; ++i) {
    result *= (n - i + 1);
    result /= i;
  }
  return result;
}

int cxy_contract(Hypergraph &hypergraph, int k) {
  // TODO this is likely to overflow when n is large (> 100000)
  long long repeat = ncr(hypergraph.num_vertices(), 2 * (k - 1));
  repeat *= std::ceil(std::log(hypergraph.num_vertices()));
  repeat = std::max(repeat, 1ll);
  int min_so_far = std::numeric_limits<int>::max();
  for (long long i = 0; i < repeat; ++i) {
    Hypergraph copy(hypergraph);
    auto start = std::chrono::high_resolution_clock::now();
    int answer_ = cxy_contract_(copy, k);
    min_so_far = std::min(min_so_far, answer_);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[" << i + 1 << "/" << repeat << "] took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                       start)
                     .count()
              << " milliseconds, got " << answer_ << "\n";
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

  if (argc != 4) {
  usage:
    std::cerr << "Usage: " << argv[0]
              << " <input hypergraph filename> <k> <algorithm>\n"
              << "  algorithm: FPZ or CXY\n";
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
  int answer;
  if (argv[3] == std::string("FPZ")) {
    answer = branching_contract(h, k);
  } else if (argv[3] == std::string("CXY")) {
    answer = cxy_contract(h, k);
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
