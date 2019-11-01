#include "hypergraph/certificate.hpp"

#include "hypergraph/order.hpp"

namespace {

// Returns the induced head ordering of the edges in O(p) time
std::vector<int> induced_head_ordering(const Hypergraph &hypergraph,
                                       std::vector<int> vertex_ordering) {
  std::unordered_map<int, size_t> vertex_to_order;
  for (size_t i = 0; i < vertex_ordering.size(); ++i) {
    vertex_to_order[vertex_ordering[i]] = i;
  }

  std::vector<std::vector<int>> buckets(vertex_ordering.size() + 1);

  for (const auto &[e, vertices] : hypergraph.edges()) {
    auto head = std::numeric_limits<size_t>::max();
    for (const int v : vertices) {
      if (vertex_to_order[v] < head) {
        head = vertex_to_order[v];
      }
    }
    buckets.at(head).push_back(e);
  }

  std::vector<int> edge_ordering;
  for (const auto &bucket : buckets) {
    edge_ordering.insert(std::end(edge_ordering), std::begin(bucket),
                         std::end(bucket));
  }

  return edge_ordering;
}

} // namespace

KTrimmedCertificate::KTrimmedCertificate(const Hypergraph &hypergraph)
    : hypergraph_(hypergraph) {
  for (const auto v : hypergraph_.vertices()) {
    backward_edges_[v] = {};
  }

  vertex_ordering_ = maximum_adjacency_ordering(
      hypergraph_, *std::begin(hypergraph.vertices()));
  std::vector<int> edge_ordering =
      induced_head_ordering(hypergraph_, vertex_ordering_);

  // Cache all heads in O(p) time
  std::unordered_map<int, size_t> vertex_to_order;
  for (size_t i = 0; i < vertex_ordering_.size(); ++i) {
    vertex_to_order[vertex_ordering_[i]] = i;
  }
  for (const auto &[e, vertices] : hypergraph.edges()) {
    edge_to_head_[e] = std::numeric_limits<size_t>::max();
    for (const int v : vertices) {
      if (vertex_to_order[v] < edge_to_head_[e]) {
        edge_to_head_[e] = vertex_to_order[v];
      }
    }
  }

  for (const auto v : hypergraph_.vertices()) {
    for (const int e : edge_ordering) {
      const auto &e_vertices = hypergraph.edges().at(e);
      if (std::find(std::begin(e_vertices), std::end(e_vertices), v) !=
          std::end(e_vertices) &&
          v != head(e)) {
        backward_edges_.at(v).push_back(e);
      }
    }
  }
}

Hypergraph KTrimmedCertificate::certificate(size_t k) const {
  // TODO Allow hypergraph map constructor without next_vertex_id
  int next_vertex_id = std::numeric_limits<int>::min();

  std::unordered_map<int, std::vector<int>> new_edges;
  std::unordered_map<int, std::vector<int>> new_vertices;
  for (const auto v : hypergraph_.vertices()) {
    next_vertex_id = std::max(next_vertex_id, v + 1);
    new_vertices.insert({v, {}});
  }

  // O(kn) loop
  for (const auto v : hypergraph_.vertices()) {
    const auto backward_edges = backward_edges_.at(v);
    for (auto it = std::begin(backward_edges);
         it != std::end(backward_edges) &&
             it != std::begin(backward_edges) + static_cast<long long>(k);
         ++it) {
      const int e = *it;
      if (new_edges.find(e) == std::end(new_edges)) {
        new_edges[e] = {head(e)};
        new_vertices.at(head(e)).push_back(e);
      }
      new_edges.at(e).push_back(v);
      new_vertices.at(v).push_back(e);
    }
  }

  return Hypergraph(std::move(new_vertices), std::move(new_edges), hypergraph_.next_vertex_id_,
                    hypergraph_.next_edge_id_);
}

int KTrimmedCertificate::head(const int e) const {
  return vertex_ordering_.at(edge_to_head_.at(e));
}
