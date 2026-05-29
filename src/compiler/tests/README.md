# Compiler Tests

Compiler tests are disabled by default. Golden tests use ApprovalTests.cpp for
approved/received output comparison, and Meson runs one fixture per test.

To build and run the tests:

```sh
cd src/compiler
# Or any other C++23 compiler
CXX=g++-13 meson setup build-tests -Dtests=true
meson compile -j 1 -C build-tests
meson test -C build-tests
```

Golden tests are discovered from these fixture directories when the build
directory is configured:

```text
src/compiler/tests/fixtures/parser/
src/compiler/tests/fixtures/validate/
src/compiler/tests/fixtures/compile/
```

Each `.rule` fixture should have a sibling `.approved.txt` file. If you add or
remove a `.rule` fixture, reconfigure the test build:

```sh
meson setup --reconfigure build-tests
```
