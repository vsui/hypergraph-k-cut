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

#include "generators.hpp"
#include "store.hpp"
#include "runner.hpp"
#include "experiment.hpp"

int run_experiment(const std::filesystem::path &config_path,
                   const std::filesystem::path &output_path = {},
                   const std::optional<size_t> &num_runs = {});
int list_sizes(const std::filesystem::path &config_path);
int check_cuts(const std::filesystem::path &config_path);
int check_approx(const std::filesystem::path &config_path);

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
      numRunsArg("r", "runs", "Override number of runs for configs", false, 0, "A positive integer", cmd);

  std::vector<TCLAP::Arg *> xor_list = {&destArg, &listSizesArg, &checkCutsArg, &checkApproxArg};

  cmd.xorAdd(xor_list);

  cmd.parse(argc, argv);

  std::filesystem::path config_path = configFileArg.getValue();
  std::filesystem::path output_path = destArg.getValue();

  const auto
      execute = [&listSizesArg, &checkCutsArg, &numRunsArg, &checkApproxArg](const std::filesystem::path &config_path,
                                                                             const std::filesystem::path &output_path) {
    try {
      if (listSizesArg.isSet()) {
        list_sizes(config_path);
      } else if (checkCutsArg.isSet()) {
        check_cuts(config_path);
      } else if (checkApproxArg.isSet()) {
        check_approx(config_path);
      } else {
        run_experiment(config_path,
                       output_path,
                       numRunsArg.isSet() ? std::make_optional(numRunsArg.getValue()) : std::nullopt);
      }
    } catch (const std::exception &e) {
      std::cerr << "Exception thrown: " << e.what() << std::endl;
      exit(1);
    }
  };

  if (std::filesystem::is_directory(config_path)) {
    if (std::filesystem::exists(output_path)) {
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

      std::filesystem::remove_all(output_path);
    }

    // Execute all config files in config_path
    for (auto &p :std::filesystem::directory_iterator(config_path)) {
      if (p.is_directory()) {
        std::cout << "Skipping " << p.path() << ", is a directory" << std::endl;
        continue;
      }
      if (p.path().extension() != ".yaml") {
        std::cout << "Skipping " << p.path() << ", is not a YAML file" << std::endl;
        continue;
      }
      execute(p.path(), output_path / p.path().stem());
    }
  } else {
    execute(config_path, output_path);
  }
}

int run_experiment(const std::filesystem::path &config_path,
                   const std::filesystem::path &output_path,
                   const std::optional<size_t> &num_runs) {
  using namespace std::string_literals;

  YAML::Node node = YAML::LoadFile(config_path);
  Experiment experiment = experiment_from_config_file(config_path);

  bool cutoff = node["cutoff"] && node["cutoff"].as<bool>();

  std::vector<std::string> algos = {};
  if (node["algos"]) {
    algos = node["algos"].as<std::vector<std::string>>();
  }

  // Prepare output directory
  if (std::filesystem::exists(output_path)) {
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

    std::filesystem::remove_all(output_path);
  }
  if (!std::filesystem::create_directories(output_path)) {
    std::cerr << "Failed to create output directory '" << output_path << "'" << std::endl;
    return 1;
  }
  std::filesystem::path db_path = output_path / "data.db";

  // Prepare logger
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(output_path / "log.txt", true);
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  std::shared_ptr<spdlog::logger> logger(new spdlog::logger("multi_sink", {file_sink, console_sink}));

  spdlog::set_default_logger(logger);

  std::filesystem::copy_file(config_path, output_path / "config.yaml");

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

  std::filesystem::path here = std::filesystem::absolute(__FILE__).remove_filename();

  std::stringstream python_cmd;
  python_cmd << "python3 "s
             << (here / ".." / ".." / (cutoff ? "scripts/sqlplot-cutoff.py" : "scripts/sqlplot.py")) << " "
             << output_path;

  std::cout << "Done, writing artifacts to " << output_path << std::endl;

  std::system(python_cmd.str().c_str());

  return 0;
}

int list_sizes(const std::filesystem::path &config_path) {
  Experiment experiment = experiment_from_config_file(config_path);
  for (const auto &gen : experiment.generators) {
    auto[hgraph, planted] = gen->generate();
    std::cout << hgraph.num_vertices() << "," << hgraph.size() << "," << cxy::CxyImpl::default_num_runs(hgraph, 2)
              << std::endl;
  }
  return 0;
}

int check_cuts(const std::filesystem::path &config_path) {
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

int check_approx(const std::filesystem::path &config_path) {
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
