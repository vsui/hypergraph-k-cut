#include <filesystem>
#include <memory>
#include <cstdlib>

#include <yaml-cpp/yaml.h>
#include <tclap/CmdLine.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <hypergraph/order.hpp>
#include <hypergraph/cxy.hpp>
#include <hypergraph/approx.hpp>

#include "store.hpp"
#include "runner.hpp"
#include "experiment.hpp"

using namespace hypergraphlib;
namespace fs = std::filesystem;

int run_experiment(const fs::path &config_path,
                   const fs::path &output_path = {},
                   const std::optional<size_t> &num_runs = {});
int list_sizes(const fs::path &config_path);
int check_cuts(const fs::path &config_path);
int check_approx(const fs::path &config_path);

// Executes different functions based on the flags
struct Executor {
  virtual int operator()(const fs::path &config_path, const fs::path &output_path) const = 0;

  virtual ~Executor() = default;

  static std::unique_ptr<Executor> factory(const bool list_sizes,
                                           const bool check_cuts,
                                           const bool check_approx,
                                           const std::optional<size_t> &num_runs);
};

struct ExperimentExecutor : public Executor {
  std::optional<size_t> num_runs;

  int operator()(const fs::path &config_path, const fs::path &output_path) const override {
    return run_experiment(config_path, output_path, num_runs);
  }
};

struct ListSizesExecutor : public Executor {
  int operator()(const fs::path &config_path, const fs::path &) const override {
    return list_sizes(config_path);
  }
};

struct CheckCutsExecutor : public Executor {

  int operator()(const fs::path &config_path, const fs::path &) const override {
    return check_cuts(config_path);
  }
};

struct CheckApproxExecutor : public Executor {
  int operator()(const fs::path &config_path, const fs::path &) const override {
    return check_approx(config_path);
  }
};

std::unique_ptr<Executor> Executor::factory(const bool list_sizes,
                                            const bool check_cuts,
                                            const bool check_approx,
                                            const std::optional<size_t> &num_runs) {
  if (list_sizes)
    return std::make_unique<ListSizesExecutor>();
  else if (check_cuts)
    return std::make_unique<CheckCutsExecutor>();
  else if (check_approx)
    return std::make_unique<CheckApproxExecutor>();
  else {
    auto exec = std::make_unique<ExperimentExecutor>();
    exec->num_runs = num_runs;
    return exec;
  }
}

void recursively_execute(const fs::path &config_path,
                         const fs::path &output_path,
                         const Executor &execute);

int main(int argc, char **argv) {
  TCLAP::CmdLine cmd("Hypergraph experiment runner", ' ', "0.1");

  TCLAP::UnlabeledValueArg<std::string> configFileArg(
      "config",
      "A path to the configuration file",
      true,
      "",
      "A path to a valid configuration file",
      cmd);

  TCLAP::UnlabeledValueArg<std::string> destArg(
      "dest",
      "Output directory for experiment artifacts",
      true,
      "",
      "A folder name");

  TCLAP::SwitchArg listSizesArg(
      "s",
      "sizes",
      "List sizes of generated hypergraphs");

  TCLAP::SwitchArg checkCutsArg(
      "c",
      "check-cuts",
      "Check that cuts are not skewed or trivial");

  TCLAP::SwitchArg checkApproxArg(
      "a",
      "check-approx",
      "Check cut factors of approximation algorithm"
  );

  TCLAP::ValueArg<size_t>
      numRunsArg("n", "runs", "Override number of runs for configs", false, 0, "A positive integer", cmd);

  TCLAP::SwitchArg recursiveArg("r", "recursive", "Run hexperiment for all configs in the tree");

  TCLAP::SwitchArg forceArg("f", "force", "Remove any files already in the output path");

  std::vector<TCLAP::Arg *> xor_list = {&destArg, &listSizesArg, &checkCutsArg, &checkApproxArg};

  cmd.xorAdd(xor_list);
  cmd.add(recursiveArg);
  cmd.add(forceArg);

  cmd.parse(argc, argv);

  fs::path config_path = configFileArg.getValue();
  fs::path output_path = destArg.getValue();

  const auto execute = Executor::factory(listSizesArg.isSet(),
                                         checkCutsArg.isSet(),
                                         checkApproxArg.isSet(),
                                         numRunsArg.isSet() ? std::make_optional(numRunsArg.getValue()) : std::nullopt);

  try {
    if (recursiveArg.isSet()) {
      if (fs::exists(output_path)) {
        if (forceArg.isSet()) {
          fs::remove_all(output_path);
        } else {
          std::cerr << "Error: '" << output_path.string() << "' already exists\n"
                    << "\nUse '-f' to force removal of '" << output_path.string() << "'" << std::endl;
          exit(1);
        }
      }
      recursively_execute(config_path, output_path, *execute);
    } else {
      (*execute)(config_path, output_path);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

int run_experiment(const fs::path &config_path,
                   const fs::path &output_path,
                   const std::optional<size_t> &num_runs) {
  using namespace std::string_literals;

  if (!fs::is_regular_file(config_path)) {
    std::cerr << "Error: " << config_path.string() << " is not a file" << std::endl;
    return 1;
  }

  YAML::Node node = YAML::LoadFile(config_path);
  Experiment experiment = experiment_from_config_file(config_path);

  bool cutoff = node["cutoff"] && node["cutoff"].as<bool>();

  std::vector<std::string> algos = {};
  if (node["algos"]) {
    algos = node["algos"].as<std::vector<std::string>>();
  }

  // Prepare output directory
  if (fs::exists(output_path)) {
    std::cout << output_path << " already exists. Overwrite? [yN]" << std::endl;

    char c;
    while (std::cin.read(&c, 1)) {
      if (c == 'y') {
        break;
      } else if (c == 'N') {
        return 0;
      } else {
        std::cout << "Enter one of [yN]" << std::endl;
      }
    }

    fs::remove_all(output_path);
  }
  if (!fs::create_directories(output_path)) {
    std::cerr << "Failed to create output directory '" << output_path << "'" << std::endl;
    return 1;
  }
  fs::path db_path = output_path / "data.db";

  // Prepare logger
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(output_path / "log.txt", true);
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  std::shared_ptr<spdlog::logger> logger(new spdlog::logger("multi_sink", {file_sink, console_sink}));

  spdlog::set_default_logger(logger);

  fs::copy_file(config_path, output_path / "config.yaml");

  // Prepare sqlite database
  auto store = std::make_shared<SqliteStore>();
  if (!store->open(db_path)) {
    std::cerr << "Failed to open store" << std::endl;
    return 1;
  }

  size_t runs = num_runs.has_value() ? num_runs.value() : node["num_runs"].as<size_t>();

  const auto factory = [&](bool cutoff, Experiment &&experiment) -> std::unique_ptr<ExperimentRunner> {
    auto &[name, generators, planted] = experiment;
    if (cutoff) {
      return std::make_unique<CutoffRunner>(name,
                                            std::move(generators),
                                            store,
                                            planted,
                                            runs,
                                            algos,
                                            node["percentages"].as<std::vector<double>>(),
                                            output_path);
    } else {
      return std::make_unique<DiscoveryRunner>(name,
                                               std::move(generators),
                                               store,
                                               planted,
                                               runs,
                                               algos);
    }
  };

  // Run experiment
  std::unique_ptr<ExperimentRunner> runner = factory(cutoff, std::move(experiment));
  runner->run();

  fs::path here = fs::absolute(__FILE__).remove_filename();

  std::cout << "Done, writing artifacts to " << output_path << std::endl;

  auto script_cmd = [here, output_path](const std::string script_name) -> std::string {
    std::stringstream python_cmd;
    python_cmd << "python3 "
               << (here / ".." / ".." / script_name);
    python_cmd << " " << output_path;
    return python_cmd.str();
  };

  std::system(script_cmd("scripts/sqlplot.py").c_str());
  if (cutoff) {
    std::system(script_cmd("scripts/sqlplot-cutoff.py").c_str());
  }

  return 0;
}

int list_sizes(const fs::path &config_path) {
  Experiment experiment = experiment_from_config_file(config_path);
  for (const auto &gen : experiment.generators) {
    auto[hgraph, planted] = gen->generate();
    std::cout << hgraph.num_vertices() << "," << hgraph.size() << "," << cxy::default_num_runs(hgraph, 2)
              << std::endl;
  }
  return 0;
}

int check_cuts(const fs::path &config_path) {
  YAML::Node node = YAML::LoadFile(config_path);
  Experiment experiment = experiment_from_config_file(config_path);

  if (node["type"].as<std::string>() != "ring" && node["k"].as<size_t>() > 2) {
    std::cerr << "ERROR: cannot check the cuts if k > 2" << std::endl;
    return 1;
  }
  for (const auto &gen : experiment.generators) {
    auto[hgraph, planted] = gen->generate();
    std::cout << "Checking " << gen->name() << std::endl;
    auto num_vertices = hgraph.num_vertices(); // Since MW_min_cut modifies hgraph
    auto cut = MW_min_cut(hgraph);
    if (cut.value == 0) {
      std::cout << gen->name() << " is disconnected" << std::endl;
    }
    auto skew = static_cast<double>(cut.partitions[0].size()) / num_vertices;
    if (skew < 0.1 || skew > 0.9) {
      std::cout << gen->name() << " has a skewed min cut (" << cut.partitions.at(0).size() << ", "
                << cut.partitions.at(1).size() << ")" << std::endl;
    } else {
      std::cout << gen->name() << " has a non-skewed min cut (" << cut.partitions.at(0).size() << ", "
                << cut.partitions.at(1).size() << ")" << std::endl;
    }
  }
  return 0;
}

int check_approx(const fs::path &config_path) {
  Experiment experiment = experiment_from_config_file(config_path);
  for (const auto &gen : experiment.generators) {
    std::cout << gen->name() << std::endl;
    const auto[hypergraph, planted] = gen->generate();
    // TODO these copies should not be necessary but right now these functions are modifying
    Hypergraph copy{hypergraph};
    auto min_cut_value = MW_min_cut_value(copy);
    for (auto epsilon : {0.1, 1., 10.}) {
      Hypergraph copy{hypergraph};
      auto approx_cut = approximate_minimizer<Hypergraph>(copy, epsilon);
      std::cout << epsilon << "," << static_cast<double>(approx_cut.value) / min_cut_value << std::endl;
    }
  }
  return 0;
}

void recursively_execute(const fs::path &config_path,
                         const fs::path &output_path,
                         const Executor &execute) {
  for (auto &p : fs::directory_iterator(config_path)) {
    if (p.is_directory()) {
      recursively_execute(p, output_path / p.path().filename(), execute);
    } else if (p.path().extension() != ".yaml") {
      std::cerr << "Skipping " << p.path() << ", extension is not .yaml" << std::endl;
    } else {
      execute(p.path(), output_path / p.path().stem());
    }
  }
}
