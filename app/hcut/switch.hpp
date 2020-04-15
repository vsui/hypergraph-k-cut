/**
 * Utilities for the hcut binary, mainly revolving around handling template instantiation for the contraction algorithms
 * with different verbosity levels
 */
#pragma once

enum class cut_algorithm {
  CXY,
  FPZ,
  MW,
  KW,
  Q,
  CX,
  KK,
};

bool is_contraction_algorithm(cut_algorithm algorithm) {
  switch (algorithm) {
  case cut_algorithm::CXY:
  case cut_algorithm::FPZ:
  case cut_algorithm::KK:return true;
  default:return false;
  }
}

template<cut_algorithm Algo>
struct DiscoverFunctor {
  template<typename HypergraphType, uint8_t verbosity>
  struct F {
    template<typename ...Args>
    static auto f(Args &&...args) {
      switch (Algo) {
      case cut_algorithm::CXY:
        return hypergraphlib::cxy::discover<HypergraphType,
                                            verbosity>(std::forward<Args>(args)...);
      case cut_algorithm::FPZ:
        return hypergraphlib::fpz::discover<HypergraphType,
                                            verbosity>(std::forward<Args>(args)...);
      case cut_algorithm::KK:return hypergraphlib::kk::discover<HypergraphType, verbosity>(std::forward<Args>(args)...);
      default:assert(false);
      }
    }
  };
};

template<cut_algorithm Algo>
struct MinCutFunctor {
  template<typename HypergraphType, uint8_t verbosity>
  struct F {
    template<typename ...Args>
    static auto f(Args &&...args) {
      switch (Algo) {
      case cut_algorithm::CXY:
        return hypergraphlib::cxy::minimum_cut<HypergraphType,
                                               verbosity>(std::forward<Args>(args)...);
      case cut_algorithm::FPZ:
        return hypergraphlib::fpz::minimum_cut<HypergraphType,
                                               verbosity>(std::forward<Args>(args)...);
      case cut_algorithm::KK:
        return hypergraphlib::kk::minimum_cut<HypergraphType,
                                              verbosity>(std::forward<Args>(args)...);
      default:assert(false);
      }
    }
  };
};

template<typename HypergraphType, template<typename, uint8_t> typename Func, typename ...Args>
auto verbosity_switch(uint8_t verbosity, Args &&...args) {
  switch (verbosity) {
  case 0:return Func<HypergraphType, 0>::f(std::forward<Args>(args)...);
  case 1:return Func<HypergraphType, 1>::f(std::forward<Args>(args)...);
  case 2:return Func<HypergraphType, 2>::f(std::forward<Args>(args)...);
  default:assert(false);
  }
}

template<typename HypergraphType, typename ...Args>
auto discovery_switch(cut_algorithm algorithm, uint8_t verbosity, Args &&...args) {
  assert(is_contraction_algorithm(algorithm));

  switch (algorithm) {
  case cut_algorithm::CXY:
    return verbosity_switch<HypergraphType, DiscoverFunctor<cut_algorithm::CXY>::F, Args...>(verbosity,
                                                                                             std::forward<
                                                                                                 Args>(
                                                                                                 args)...);
  case cut_algorithm::FPZ:
    return verbosity_switch<HypergraphType, DiscoverFunctor<cut_algorithm::FPZ>::F, Args...>(verbosity,
                                                                                             std::forward<
                                                                                                 Args>(
                                                                                                 args)...);
  case cut_algorithm::KK:
    return verbosity_switch<HypergraphType, DiscoverFunctor<cut_algorithm::KK>::F, Args...>(verbosity,
                                                                                            std::forward<
                                                                                                Args>(
                                                                                                args)...);
  default:assert(false);
  }
}

template<typename HypergraphType, typename ...Args>
auto mincut_switch(cut_algorithm algorithm, uint8_t verbosity, Args &&...args) {
  assert(is_contraction_algorithm(algorithm));

  switch (algorithm) {
  case cut_algorithm::CXY:
    return verbosity_switch<HypergraphType, MinCutFunctor<cut_algorithm::CXY>::F, Args...>(verbosity,
                                                                                           std::forward<
                                                                                               Args>(
                                                                                               args)...);
  case cut_algorithm::FPZ:
    return verbosity_switch<HypergraphType, MinCutFunctor<cut_algorithm::FPZ>::F, Args...>(verbosity,
                                                                                           std::forward<
                                                                                               Args>(
                                                                                               args)...);
  case cut_algorithm::KK:
    return verbosity_switch<HypergraphType, MinCutFunctor<cut_algorithm::KK>::F, Args...>(verbosity,
                                                                                          std::forward<
                                                                                              Args>(
                                                                                              args)...);
  default:assert(false);
  }
}
