#include <iostream>
#include <fstream>

#include "hypergraph/hypergraph.hpp"
#include "hypergraph/certificate.hpp"

bool hmetis_file_is_unweighted(const std::string &filename) {
  std::ifstream input;
  input.open(filename);
  return is_unweighted_hmetis_file(input);
}

bool parse_hypergraph(const std::string &filename, Hypergraph &hypergraph) {
  std::ifstream input;
  input.open(filename);
  input >> hypergraph;
  return input.eof();
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <input file> <k>" << std::endl;
    return 1;
  }

  int k = std::atoi(argv[2]);

  if (!hmetis_file_is_unweighted(argv[1])) {
    std::cerr << argv[1] << " is not an unweighted hmetis file." << std::endl;
    return 1;
  }

  Hypergraph hypergraph;
  if (!parse_hypergraph(argv[1], hypergraph)) {
    std::cout << "Failed to parse hypergraph in " << argv[1] << std::endl;
    return 1;
  }

  // Sparsify
  KTrimmedCertificate certificate(hypergraph);

  Hypergraph sparse = certificate.certificate(k);

  std::string new_filename = std::string("sparse_") + argv[1];

  std::ofstream stream;
  stream.open(new_filename);

  stream << sparse;

  stream.close();

  std::cout << new_filename << std::endl;

  return 0;
}
