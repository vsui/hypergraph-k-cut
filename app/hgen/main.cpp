#include <variant>

#include <tclap/CmdLine.h>
#include <generators/generators.hpp>

TCLAP::CmdLine cmd("Hypergraph instance generator", ' ', "0.1");

struct Param {
  struct Description {
    std::string flag; // May be empty string if there is no flag
    std::string name;
    std::string description;
  };

  Description description;

  template<typename T>
  static Param make(std::string short_name, std::string name, std::string description) {
    return make<T>({.flag = std::move(short_name), .name = std::move(name), .description = std::move(description)});
  }

  template<typename T>
  static Param make(std::string name, std::string description) {
    return make<T>({.flag = "", .name = std::move(name), .description = std::move(description)});
  }

  template<typename T>
  static Param make(const Description &desc) {
    auto const arg = new TCLAP::ValueArg<T>(desc.flag, desc.name, desc.description, false, {}, "", cmd);
    return Param(desc, std::move(arg));
  }

  // T_name should be a human-readable name for the template parameter `T`
  template <typename T>
  T getValue(const std::string &T_name) const {
    try {
      return std::get<TCLAP::ValueArg<T> *>(arg)->getValue();
    } catch (std::exception const &) {
      std::string msg = "Internal error: Error converting " + description.name + " to " + T_name;
      throw std::invalid_argument(msg);
    }
  }

  operator size_t() const {
    return getValue<size_t>("size_t");
  }

  operator double() const {
    return getValue<double>("double");
  }

  ~Param() {
    // TODO Causes a segfault
    //std::visit([](auto p) { delete p; }, arg);
  }

private:
  template<typename T>
  Param(Description desc, T &&arg)
      : description(std::move(desc)),
        arg(std::forward<T>(arg)) {};

  // TCLAP::ValueArg not movable or copyable, so work with pointers.
  std::variant<
      TCLAP::ValueArg<size_t> *,
      TCLAP::ValueArg<double> *
  > arg;
};

int main(int argc, char *argv[]) {
  using namespace std::string_literals;

  auto const params = std::map<std::string, Param>(
      {
          {"num_vertices", Param::make<size_t>("n", "num_vertices", "Number of vertices")},
          {"num_edges", Param::make<size_t>("m", "num_edges", "Number of edges")},
          {"rank", Param::make<size_t>("r", "rank", "Rank of each edge")},
          {"seed", Param::make<size_t>("s", "seed", "Random seed")},
          {"mean", Param::make<double>("mean", "Mean angle of each sector")},
          {"m1", Param::make<size_t>("m1", "Number of intercluster edges")},
          {"m2", Param::make<size_t>("m2", "Number of intracluster edges")},
          {"p1", Param::make<double>("p1", "Pick each vertex in the cluster with probability p1 for the intercluster edges")},
          {"p2",Param::make<double>("p2", "Pick each vertex in the cluster with probability p2 for the intracluster edges")},
          {"num_clusters", Param::make<size_t>("k", "num_clusters", "Number of clusters")}
      });

  std::vector<std::string> allowed = {"planted", "planted_constant_rank", "ring"};
  TCLAP::ValuesConstraint<std::string> allowedInstances(allowed);
  TCLAP::UnlabeledValueArg<std::string>
      instanceArg("instance", "Type of instance to generate", true, "", &allowedInstances, cmd);

  cmd.parse(argc, argv);

  const std::string instance = instanceArg.getValue();

  if (instance == "planted") {
    PlantedHypergraph generator(
        params.at("num_vertices"),
        params.at("m1"),
        params.at("p1"),
        params.at("m2"),
        params.at("p2"),
        params.at("k"),
        static_cast<size_t>(params.at("seed"))
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else if (instance == "planted_constant_rank") {
    UniformPlantedHypergraph generator(
        params.at("num_vertices"),
        params.at("num_clusters"),
        params.at("rank"),
        params.at("m1"),
        params.at("m2"),
        static_cast<size_t>(params.at("seed"))
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else if (instance == "ring") {
    RandomRingConstantEdgeHypergraph generator(
        params.at("num_vertices"),
        params.at("num_edges"),
        params.at("mean"),
        static_cast<size_t>(params.at("seed"))
    );
    auto[hypergraph, _] = generator.generate();
    std::cout << hypergraph;
  } else {
    std::cerr << "No such instance '" << instance << "'" << std::endl;
    return 1;
  }
}
