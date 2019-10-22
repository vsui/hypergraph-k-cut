#include "certificate.h"

#include "order.h"

namespace {

// Returns the induced head ordering of the edges
std::vector<int> induced_head_ordering(const Hypergraph &hypergraph,
                                       std::vector<int> vertex_ordering) {
  std::unordered_map<int, int> vertex_to_order;
  for (int i = 0; i < vertex_ordering.size(); ++i) {
    vertex_to_order[vertex_ordering[i]] = i;
  }

  std::vector<std::vector<int>> buckets(vertex_ordering.size() + 1);

  for (const auto &[e, vertices] : hypergraph.edges()) {
    int head = std::numeric_limits<int>::max();
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

}

KTrimmedCertificate::KTrimmedCertificate(const Hypergraph &hypergraph)
    : hypergraph_(hypergraph) {
  for (const auto &[v, edges] : hypergraph_.vertices()) {
    backward_edges_[v] = {};
  }

  vertex_ordering_ = maximum_adjacency_ordering(
      hypergraph_, std::begin(hypergraph.vertices())->first);
  std::vector<int> edge_ordering =
      induced_head_ordering(hypergraph_, vertex_ordering_);

  for (const auto &[v, edges] : hypergraph_.vertices()) {
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

Hypergraph KTrimmedCertificate::certificate(const int k) const {
  std::unordered_map<int, std::vector<int>> new_edges;

  for (const auto &[v, edges] : hypergraph_.vertices()) {
    const auto backward_edges = backward_edges_.at(v);
    for (auto it = std::begin(backward_edges);
         it != std::end(backward_edges) && it != std::begin(backward_edges) + k;
         ++it) {
      const int e = *it;
      if (new_edges.find(e) == std::end(new_edges)) {
        new_edges[e] = {head(e)};
      }
      new_edges.at(e).push_back(v);
    }
  }

  std::vector<int> certificate_vertices;
  std::vector<std::vector<int>> certificate_edges;

  for (const auto &[v, edges] : hypergraph_.vertices()) {
    certificate_vertices.push_back(v);
  }
  for (const auto &[e, vertices] : new_edges) {
    certificate_edges.push_back(std::move(vertices));
  }

  return Hypergraph(certificate_vertices, certificate_edges);
}

int KTrimmedCertificate::head(const int e) const {
  // TODO return find with cached vertex_to_pos lookups
  return *std::min_element(
      std::begin(hypergraph_.edges().at(e)),
      std::end(hypergraph_.edges().at(e)), [this](const int a, const int b) {
        return std::distance(std::find(std::begin(vertex_ordering_),
                                       std::end(vertex_ordering_), a),
                             std::find(std::begin(vertex_ordering_),
                                       std::end(vertex_ordering_), b)) > 0;
      });
}
