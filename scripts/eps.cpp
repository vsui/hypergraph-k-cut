/**
 * Dumps out a csv file with information on ring hypergraphs
 */

#include <ostream>
#include <istream>
#include <iostream>
#include <chrono>
#include <sstream>
#include <vector>

#include <boost/hana.hpp>
#include <hypergraph/approx.hpp>
#include <generators/generators.hpp>
#include <hypergraph/certificate.hpp>

namespace hana = boost::hana;

struct InputInfo {
  BOOST_HANA_DEFINE_STRUCT(
      InputInfo,
      (size_t, num_vertices),
      (size_t, num_edges),
      (double, radius),
      (uint64_t, seed),
      (double, epsilon),

      (size_t, min_cut_value),
      (size_t, eps_cut_value),
      (size_t, size_before),
      (size_t, size_after),
      (size_t, p1),
      (size_t, p2)
  );
  /*
  size_t num_vertices;
  size_t num_edges;
  double radius;
  double epsilon;
  // Suboptimality factor of cut found by running the approximate algorithm
  double suboptimality_factor;
  size_t size_before;
  // Size after sparsifying with the cut value found by the approximate algorithm
  size_t size_after;
  // Size of partition 1
  size_t p1;
  // Size of partition 2
  size_t p2;
   */

  static std::string header() {
    std::stringstream s;
    hana::for_each(InputInfo{}, hana::fuse([&s](const auto &name, const auto &member) {
      s << name.c_str() << ",";
    }));
    std::string str = s.str();
    return str.substr(0, str.size() - 1);
  }
};

// Comma delimit
template<typename T>
std::string comma_delimit(const T &data) {
  std::stringstream s;
  hana::for_each(data, hana::fuse([&s](const auto &name, const auto &member) {
    s << member << ",";
  }));
  std::string str = s.str();
  return str.substr(0, str.size() - 1);
}

std::ostream &operator<<(std::ostream &out, const InputInfo &info) {
  out << comma_delimit(info) << std::endl;
  return out;
}

/*
std::istream &operator>>(std::istream &in, InputInfo &info) {
  std::string line;
  std::getline(in, line);
  std::stringstream line_stream(line);
  auto map = hana::to<hana::map_tag>(info);
  hana::for_each(info, hana::fuse([&line_stream, &map](auto name, auto member) {
    std::string field;
    std::getline(line_stream, field, ',');
    std::stringstream field_stream(field);
    decltype(member) m;
    field_stream >> m;
    map[name] = m;
  }));

  info.num_vertices = map[BOOST_HANA_STRING("num_vertices")];
  info.num_edges = map[BOOST_HANA_STRING("num_edges")];

  return in;
}
 */

struct Timer {
  using TimePoint = decltype(std::chrono::high_resolution_clock::now());

  Timer() {
    start = std::chrono::high_resolution_clock::now();
  }

  auto stop() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
  }
private:
  TimePoint start;
};

// Compare just the input parts of InputInfo
struct CompareInput {
  static auto to_tuple(const InputInfo &a) {
    return std::make_tuple(a.num_vertices, a.num_edges, a.radius);
  }

  bool operator()(const InputInfo &a, const InputInfo &b) const {
    return to_tuple(a) < to_tuple(b);
  }
};

hypergraphlib::HypergraphCut<size_t> memoized_cut(const InputInfo &info, const hypergraphlib::Hypergraph &hypergraph) {
  static std::map<InputInfo, hypergraphlib::HypergraphCut<size_t>, CompareInput> cuts;

  auto it = cuts.find(info);
  if (it == cuts.end()) {
    hypergraphlib::Hypergraph h(hypergraph);
    const auto cut = hypergraphlib::MW_min_cut(h);
    cuts.insert({info, cut});
    return cut;
  }
  return it->second;
}

int main() {
  std::vector<InputInfo> infos;

  for (size_t num_vertices = 100; num_vertices <= 500; num_vertices += 25) {
    for (size_t num_edges = 100; num_edges <= num_vertices * 30; num_edges += 250) {
      for (double radius = 5; radius <= 90; radius += 5) {
        for (double epsilon: {0.1, 0.2, 0.4, 0.8, 1., 2., 4., 8., 16., 32., 64., 128.}) {
          for (int i = 0; i < 10; ++i) {
            std::mt19937_64 rd;
            std::uniform_int_distribution<uint64_t> dis;
            uint64_t seed;
            InputInfo info = {
                .num_vertices = num_vertices,
                .num_edges = num_edges,
                .radius = radius,
                .seed = dis(rd),
                .epsilon = epsilon
            };
            infos.emplace_back(std::move(info));
          }
        }
      }
    }
  }

  std::cout << InputInfo::header() << std::endl;

  for (auto &info : infos) {
    RandomRingConstantEdgeHypergraph gen(info.num_vertices, info.num_edges, info.radius, 777);
    auto[h, _] = gen.generate();
    // TODO do this before in generate or something
    h.remove_singleton_and_empty_hyperedges();
    const hypergraphlib::Hypergraph hypergraph(h);

    const auto cut = memoized_cut(info, hypergraph);
    hypergraphlib::Hypergraph temp2(hypergraph);
    const auto eps_cut = hypergraphlib::approximate_minimizer(temp2, info.epsilon);
    hypergraphlib::KTrimmedCertificate k(hypergraph);
    auto hypergraph_after = k.certificate(eps_cut.value);

    info.eps_cut_value = eps_cut.value;
    info.min_cut_value = cut.value;
    info.size_before = hypergraph.size();
    info.size_after = hypergraph_after.size();
    info.p1 = cut.partitions.at(0).size();
    info.p2 = cut.partitions.at(1).size();

    std::cout << info;
  }
}
