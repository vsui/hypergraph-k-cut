#include <boost/hana.hpp>
#include <tclap/CmdLine.h>
#include <generators/generators.hpp>

namespace hana = boost::hana;

template<typename T>
struct Param {
  using type = T;

  Param(std::string short_name, std::string name, std::string description)
      : short_name(std::move(short_name)), name(std::move(name)), description(std::move(description)) {}

  Param(std::string name, std::string description)
      : name(std::move(name)), description(std::move(description)) {}

  std::string short_name;
  std::string name;
  std::string description;

  TCLAP::ValueArg<T> *arg;
};

struct Foo {
  BOOST_HANA_DEFINE_STRUCT(
      Foo,
      (size_t, num_vertices),
      (size_t, num_edges)
  );
};

struct Bar {
  BOOST_HANA_DEFINE_STRUCT(
      Bar,
      (size_t, num_vertices),
      (size_t, num_edges),
      (size_t, seed)
  );
};

int main(int argc, char *argv[]) {
  using namespace hana::literals;
  Foo foo;
  Bar bar;

  auto params = hana::make_tuple(
      Param<size_t>("n", "num_vertices", "Number of vertices"),
      Param<size_t>("m", "num_edges", "Number of edges"),
      Param<size_t>("r", "rank", "Rank of each edge"),
      Param<size_t>("s", "seed", "Random seed"),
      Param<double>("mean", "Mean angle of each sector"),
      Param<double>("a", "radius", "Radius of the size of each sector"),
      Param<size_t>("m1", "Number of intercluster edges"),
      Param<size_t>("m2", "Number of intracluster edges"),
      Param<double>("p1", "Pick each vertex in the cluster with probability p1 for the intercluster edges"),
      Param<double>("p2", "Pick each vertex in the cluster with probability p2 for the intracluster edges"),
      Param<size_t>("k", "num_clusters", "Number of clusters")
  );

  const auto blah = [&params]() {
    hana::find_if(params, hana::equal.to(hana::type_c<unsigned>));
  };

  //const auto get_param = [&params]( std::string name) {
  //  const auto param = hana::find_if(params, [&name](const auto &param) { return name = params.name.c_str(); });
  //  if (it == hana::nothing) {
  //    std::cerr << "Internal error: no such param '" << name << "'";
  //    throw std::exception("Internal error");
  //  }
  //  return param.arg->getValue();
  //};

  std::vector<std::string> allowed = {"planted", "planted_constant_rank", "ring"};
  TCLAP::ValuesConstraint<std::string> allowedInstances(allowed);
  TCLAP::CmdLine cmd("Hypergraph instance generator", ' ', "0.1");
  TCLAP::UnlabeledValueArg<std::string>
      instanceArg("instance", "Type of instance to generate", true, "", &allowedInstances, cmd);

  hana::for_each(params, [&cmd](auto param) {
    param.arg =
        new TCLAP::ValueArg<typename decltype(param)::type>(param.short_name,
                                                            param.name,
                                                            param.description,
                                                            false,
                                                            {},
                                                            "",
                                                            cmd);
    std::cout << param.name << (param.arg == nullptr) << std::endl;
  });

  cmd.parse(argc, argv);

  const std::string instance = instanceArg.getValue();
  if (instance == "planted") {
    PlantedHypergraph generator(
        hana::at(params, 0_c).arg->getValue(),
        hana::at(params, 6_c).arg->getValue(),
        hana::at(params, 8_c).arg->getValue(),
        hana::at(params, 7_c).arg->getValue(),
        hana::at(params, 9_c).arg->getValue(),
        hana::at(params, 10_c).arg->getValue(),
        777
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else if (instance == "planted_constant_rank") {
    UniformPlantedHypergraph generator(
        hana::at(params, 0_c).arg->getValue(),
        hana::at(params, 10_c).arg->getValue(),
        hana::at(params, 2_c).arg->getValue(),
        hana::at(params, 6_c).arg->getValue(),
        hana::at(params, 7_c).arg->getValue(),
        777
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else if (instance == "ring") {
    hana::for_each(params, [&cmd](auto param) {
      std::cout << "2 " << param.name << " " << (param.arg == nullptr) << std::endl;
    });
    RandomRingConstantEdgeHypergraph generator(
        /* hana::at(params, 0_c).arg->getValue() */ 10,
        /* hana::at(params, 1_c).arg->getValue() */ 10,
        /* hana::at(params, 4_c).arg->getValue() */ 10,
                                                    777
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else {
    std::cerr << "No such instance '" << instance << "'" << std::endl;
    return 1;
  }
}