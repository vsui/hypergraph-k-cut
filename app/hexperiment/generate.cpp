//
// Created by Victor on 2/24/20.
//

#include <tuple>
#include <iostream>

#include "generators.hpp"

namespace {
template<typename... Ts>
struct Impl;

template<typename T, typename... Ts>
struct Impl<T, Ts...> {
  static std::tuple<T, Ts...> read_tuple() {
    T t{};
    std::cin >> t;
    std::tuple<T> head{t};
    std::tuple<Ts...> tail = Impl<Ts...>::read_tuple();
    return std::tuple_cat(head, tail);
  }
};

template<>
struct Impl<> {
  static std::tuple<> read_tuple() {
    return {};
  }
};

template<typename... Ts>
std::tuple<Ts...> read_tuple() {
  return Impl<Ts...>::read_tuple();
}

}

int main() {
  read_tuple<int, int, double>();
  return 0;
}
