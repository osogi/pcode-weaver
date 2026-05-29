#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

namespace golden {

class ScopedCerrRedirect {
public:
  explicit ScopedCerrRedirect(std::ostream &replacement);
  ~ScopedCerrRedirect();

  ScopedCerrRedirect(const ScopedCerrRedirect &) = delete;
  ScopedCerrRedirect &operator=(const ScopedCerrRedirect &) = delete;

private:
  std::streambuf *originalBuffer;
};

std::filesystem::path displayPath(const std::filesystem::path &fixturePath);
int verifyFixtureOutput(
    const std::filesystem::path &fixturePath, const std::string &output
);

} // namespace golden
