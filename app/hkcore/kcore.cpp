/**
 * This is a binary for finding interesting minimum cuts in a hypergraph. An interesting cut is a cut where
 * neither of the partitions have only one vertex (in other words, the cut is not skewed)
 *
 * Whenever an interesting cut is found, it will output a file "<filename>.cut", where <filename> is the root of the
 * hypergraph filename. This `cut` file will contain a description of the cut.
 *
 * This binary will also take k-core decompositions of the hypergraphs if they are not yielding interesting cuts.
 * When an interesting cut of an interesting k-core decomposition is found, a file <filename>.<k>core.cut will be written,
 * along with a file containing the actual decomposition <filename>.<k>core.hgr.
 */

#include <fstream>
#include <filesystem>

#include <hypergraph/hypergraph.hpp>
#include <hypergraph/order.hpp>

using std::begin, std::end;
using namespace hypergraphlib;

template<typename HypergraphType>
int run(const HypergraphType &h, const std::string &filename, const std::filesystem::path out_path) {
  constexpr auto isTrivial = [](auto &part) {
    return part.size() == 1;
  };

  auto rank = h.rank();

  for (size_t k = 2; k < rank; ++k) {
    std::cout << "Computing k-core decomposition with k = " << k << "..." << std::endl;
    auto kcore = kCoreDecomposition(h, k);

    if (kcore.num_vertices() == h.num_vertices()) {
      std::cout << "Decomposition did not reduce graph, continuing..." << std::endl;
      continue;
    }

    std::cout << "Computed k-core decomposition\n"
              << "Vertices: " << h.num_vertices() << " -> " << kcore.num_vertices() << "\n"
              << "Edges: " << h.num_edges() << " -> " << kcore.num_edges() << std::endl;

    std::cout << "Computing minimum cut..." << std::endl;
    auto kcore_copy = HypergraphType{kcore};
    auto cut = MW_min_cut(kcore);
    auto &parts = cut.partitions;

    if (std::any_of(begin(parts), end(parts), isTrivial)) {
      std::cout << "Cut was trivial" << std::endl;
      continue;
    }

    // Write k-core and cut to file
    std::filesystem::path p(filename);

    std::string cut_filename = p.filename().stem().string() + "." + std::to_string(k) + "core.cut";
    std::string kcore_filename = p.filename().stem().string() + "." + std::to_string(k) + "core.hgr";

    std::cout << "Interesting cut found, writing to " << cut_filename << " and " << kcore_filename << std::endl;

    std::ofstream cut_file;
    cut_file.open(out_path / cut_filename);
    cut_file << cut;

    std::ofstream kcore_file;
    kcore_file.open(out_path / kcore_filename);
    kcore_file << kcore_copy;
  }
  std::cout << "Done with all" << std::endl;

  return 0;
}

bool hmetis_file_is_unweighted(const std::string &filename) {
  std::ifstream input;
  input.open(filename);
  return is_unweighted_hmetis_file(input);
}

// Attempts to read a hypergraph from a file
template<typename HypergraphType>
bool parse_hypergraph(const std::string &filename, HypergraphType &hypergraph) {
  std::ifstream input;
  input.open(filename);
  input >> hypergraph;
  return true;
}

int main(int argc, char **argv) {
  if (hmetis_file_is_unweighted(argv[1])) {
    Hypergraph h;
    assert(parse_hypergraph(argv[1], h));
    run<Hypergraph>(h, argv[1], argv[2]);
  } else {
    assert(false);
  }
}
