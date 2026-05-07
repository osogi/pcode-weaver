// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include <expected>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

class Error {
public:
  Error(const std::string &msg) : msg(msg) {}
  Error(std::string &&msg) : msg(std::move(msg)) {}

  const std::string &message() const { return msg; }

private:
  std::string msg;
};

template <typename CONCRETE_VALUE_TYPE>
class [[nodiscard]] Errorable
    : public std::expected<CONCRETE_VALUE_TYPE, Error> {
  using Base = std::expected<CONCRETE_VALUE_TYPE, Error>;

public:
  using Base::Base;
  using Base::operator=;

  template <typename Func> auto and_then(Func &&func) & {
    if constexpr (std::is_void_v<CONCRETE_VALUE_TYPE>) {
      using Ret = std::invoke_result_t<Func>;
      if (!this->has_value()) {
        return Ret(std::unexpected(this->error()));
      }
      return std::invoke(std::forward<Func>(func));
    } else {
      using Ret = std::invoke_result_t<Func, CONCRETE_VALUE_TYPE &>;
      if (!this->has_value()) {
        return Ret(std::unexpected(this->error()));
      }
      return std::invoke(std::forward<Func>(func), this->value());
    }
  }

  template <typename Func> auto and_then(Func &&func) const & {
    if constexpr (std::is_void_v<CONCRETE_VALUE_TYPE>) {
      using Ret = std::invoke_result_t<Func>;
      if (!this->has_value()) {
        return Ret(std::unexpected(this->error()));
      }
      return std::invoke(std::forward<Func>(func));
    } else {
      using Ret = std::invoke_result_t<Func, const CONCRETE_VALUE_TYPE &>;
      if (!this->has_value()) {
        return Ret(std::unexpected(this->error()));
      }
      return std::invoke(std::forward<Func>(func), this->value());
    }
  }

  template <typename Func> auto and_then(Func &&func) && {
    if constexpr (std::is_void_v<CONCRETE_VALUE_TYPE>) {
      using Ret = std::invoke_result_t<Func>;
      if (!this->has_value()) {
        return Ret(std::unexpected(std::move(this->error())));
      }
      return std::invoke(std::forward<Func>(func));
    } else {
      using Ret = std::invoke_result_t<Func, CONCRETE_VALUE_TYPE &&>;
      if (!this->has_value()) {
        return Ret(std::unexpected(std::move(this->error())));
      }
      return std::invoke(std::forward<Func>(func), std::move(this->value()));
    }
  }
};

template <typename... Args> std::unexpected<Error> err(Args &&...args) {
  return std::unexpected(Error(std::forward<Args>(args)...));
}
