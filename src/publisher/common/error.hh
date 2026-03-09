#pragma once

#include <expected>
#include <string>

class Error {
public:
  Error(const std::string& msg) : msg(msg) {}
  Error(std::string&& msg) : msg(std::move(msg)) {}

  const std::string& message() const {
    return msg;
  }
private:
  std::string msg;
};

template <typename CONCRETE_VALUE_TYPE>
using Errorable = std::expected<CONCRETE_VALUE_TYPE, Error>;

template<typename... Args>
std::unexpected<Error> err(Args&&... args) {
    return std::unexpected(Error(std::forward<Args>(args)...));
}