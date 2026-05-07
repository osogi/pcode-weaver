// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

namespace util {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace util

template <typename T, typename Func>
auto map_vector(const std::vector<T> &source, Func &&func) {
  // Deduce the return type of the function to create the correct vector type
  using ResultType = std::invoke_result_t<Func, const T &>;
  std::vector<ResultType> result;

  if (source.empty()) {
    return result;
  }

  result.reserve(source.size());

  std::transform(
      source.begin(),
      source.end(),
      std::back_inserter(result),
      std::forward<Func>(func)
  );

  return result;
}

template <class T> inline void hash_combine(std::size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class Alt, class VariantType> Alt *get_if_uniq(VariantType *variant) {
  auto *p = std::get_if<std::unique_ptr<Alt>>(variant);
  if (p != nullptr) {
    return p->get();
  }
  return nullptr;
}

template <class Alt, class... Alts>
Alt *get_if_force(std::variant<Alts...> *variant) {
  if (!variant)
    throw std::invalid_argument("get_if_force: null variant pointer");

  if (auto *p = std::get_if<Alt>(variant))
    return p;

  std::string msg = "get_if_force: variant does not hold requested type ";
  msg += typeid(Alt).name();

  throw std::runtime_error(msg);
};

template <typename... Ts>
std::ostream &printVariant(std::ostream &os, const std::variant<Ts...> &var) {
  std::visit([&os](const auto &val) { os << val; }, var);
  return os;
}