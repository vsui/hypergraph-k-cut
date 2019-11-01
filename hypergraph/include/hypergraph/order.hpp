#pragma once

#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <boost/heap/fibonacci_heap.hpp>

#include "hypergraph/bucket_list.hpp"
#include "hypergraph/certificate.hpp"
#include "hypergraph/hypergraph.hpp"

// A context for vertex ordering calculations
struct OrderingContext {
    // Constructor
    // OrderingContext(BucketList &&blist);

    // Heap for tracking which vertices are most tightly connected to the
    // ordering
    boost::heap::fibonacci_heap<std::pair<size_t, int>> heap;

    std::unordered_map<
            int, boost::heap::fibonacci_heap<std::pair<size_t, int>>::handle_type>
            handles;

    // A mapping of edges to the number of vertices inside the edges that have not
    // been ordered
    std::unordered_map<int, int> edge_to_num_vertices_outside_ordering;

    // Vertices that have been marked as used by the ordering
    std::unordered_set<int> used_vertices;

    // Edges that have been marked as used by the ordering
    std::unordered_set<int> used_edges;
};

/* The method for calculating a vertex ordering for maximum adjacency and tight,
 * and Queyranne orderings is very similar - maintain how "tight" each vertex is
 * and repeatedly add the "tightest" vertex. The only difference is how
 * "tightness" is defined for each ordering. The "_tighten" functions define how
 * each ordering maintains tightness.
 *
 * Each of these methods runs in time linear to the number of edges incident on
 * v.
 *
 * TODO these don't need to be exposed in the header anymore
 */
void maximum_adjacency_ordering_tighten(const Hypergraph &hypergraph,
                                        OrderingContext &ctx, const int v);

void tight_ordering_tighten(const Hypergraph &hypergraph, OrderingContext &ctx,
                            const int v);

void queyranne_ordering_tighten(const Hypergraph &hypergraph,
                                OrderingContext &ctx, const int v);

using tightening_t =
std::add_pointer_t<void(const Hypergraph &, OrderingContext &, const int)>;

/* Given a method to update the "tightness" of different vertices, computes a
 * vertex ordering and returns the ordering as well as a list of how tight each
 * vertex was when it was added to the ordering.
 *
 * Time complexity: O(p), where p is the size of the hypergraph, assuming that
 * TIGHTEN runs in time linear to the number of edges incident on the selected
 * vertex.
 */
template<tightening_t TIGHTEN>
std::pair<std::vector<int>, std::vector<double>>
unweighted_ordering(const Hypergraph &hypergraph, const int a) {
    std::vector<int> ordering = {a};
    std::vector<double> tightness = {0};
    std::vector<int> vertices_without_a;

    for (const auto v : hypergraph.vertices()) {
        if (v == a) {
            continue;
        }
        vertices_without_a.emplace_back(v);
    }

    // Initialize context
    // TODO I would like to initialize the bucketlist directly in the ctx arg list
    // so i don't have a name dangling around (blist) but for some reason this
    // causes a runtime error
    // BucketList blist(vertices_without_a,
    //                 2 * hypergraph.edges().size() +
    //                     1 /* multiply by 2 for Queyranne ordering */);
    OrderingContext ctx;
    for (const int v : vertices_without_a) {
        auto handle = ctx.heap.push({0, v});
        ctx.handles[v] = handle;
    }
    for (const auto &[e, vertices] : hypergraph.edges()) {
        ctx.edge_to_num_vertices_outside_ordering.insert({e, vertices.size()});
    }

    const auto tighten = [&hypergraph, &ctx](const int v) {
        ctx.used_vertices.insert(v);
        // It is the responsibility of TIGHTEN to update the context
        TIGHTEN(hypergraph, ctx, v);
    };

    tighten(a);

    while (ordering.size() < hypergraph.num_vertices()) {
        const auto[k, v] = ctx.heap.top();
        ctx.heap.pop();
        ordering.emplace_back(v);
        // We need k / 2 instead of just k because this is just used for Queyranne
        // for now
        tightness.push_back(k / 2.0);
        tighten(v);
    }

    return {ordering, tightness};
}

/* Returns a maximum adjacency ordering of vertices, starting with vertex a.
 * Linear in the number of vertices across all hyperedges.
 *
 * Here, tightness for a vertex v is the number of edges that intersect the
 * ordering so far and v.
 */
std::vector<int> maximum_adjacency_ordering(const Hypergraph &hypergraph,
                                            const int a);

/* Return a tight ordering of vertices, starting with vertex a. Linear in the
 * number of vertices across all hyperedges.
 *
 * Here, tightness is the number of edges connecting a vertex v to the
 * ordering so far that consist of vertices either in the ordering or v
 * itself.
 *
 * Takes time linear with the size of the hypergraph.
 */
std::vector<int> tight_ordering(const Hypergraph &hypergraph, const int a);

/* Return a Queyranne ordering of vertices, starting with vertex a.
 *
 * Takes time linear with the size of the hypergraph.
 */
std::vector<int> queyranne_ordering(const Hypergraph &hypergraph, const int a);

constexpr auto queyranne_ordering_with_tightness =
        unweighted_ordering<queyranne_ordering_tighten>;

size_t one_vertex_cut(const Hypergraph &hypergraph, const int v);

/* Return a new hypergraph with vertices s and t merged.
 *
 * Time complexity: O(p), where p is the size of the hypergraph
 */
Hypergraph merge_vertices(const Hypergraph &hypergraph, const int s,
                          const int t);

/* Given a hypergraph and a function that orders the vertices, find the min cut
 * by repeatedly finding and contracting pendant pairs.
 *
 * Time complexity: O(np), where n is the number of vertices and p is the size
 * of the hypergraph
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template<typename Ordering>
size_t vertex_ordering_mincut(Hypergraph &hypergraph, const int a, Ordering f) {
    size_t min_cut_of_phase = std::numeric_limits<size_t>::max();
    while (hypergraph.num_vertices() > 1) {
        auto ordering = f(hypergraph, a);
        size_t cut_of_phase = one_vertex_cut(hypergraph, ordering.back());
        hypergraph = merge_vertices(hypergraph, *(std::end(ordering) - 2),
                                    *(std::end(ordering) - 1));
        min_cut_of_phase = std::min(min_cut_of_phase, cut_of_phase);
    }
    return min_cut_of_phase;
}

/* Given a hypergraph and a function that orders the vertices, find the minimum
 * cut through an exponential search on the minimum cuts of k-trimmed
 * certificates. See [CX'09] for more details.
 *
 * Time complexity: O(p + cn^2), where p is the size of the hypergraph, c is the
 * value of the minimum cut, and n is the number of vertices
 *
 * Ordering should be one of `tight_ordering`, `queyranne_ordering`, or
 * `maximum_adjacency_ordering`.
 */
template<typename Ordering>
size_t vertex_ordering_mincut_certificate(Hypergraph &hypergraph, const int a,
                                          Ordering f) {
    KTrimmedCertificate gen(hypergraph);
    size_t k = 1;
    while (true) {
        // Copy hypergraph
        std::cout << "Trying k=" << k << std::endl;
        Hypergraph certificate = gen.certificate(k);
        std::cout << "Tried k=" << k << std::endl;
        size_t mincut = vertex_ordering_mincut(certificate, a, f);
        if (mincut < k) {
            return mincut;
        }
        k *= 2;
    }
}
