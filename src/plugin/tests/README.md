# Plugin System Tests

These tests run the plugin through a fixed Ghidra/ReOxide Docker environment and
compare the raw decompiled `main` output with ApprovalTests files.

Build the reusable base image once:

```sh
docker build \
  -f src/plugin/tests/docker/base.Dockerfile \
  -t pcode-weaver-plugin-test-base:ghidra-12.0_reoxide-0.7.2 \
  .
```

Build the per-checkout test image:

```sh
docker build --target plugin-test -t pcode-weaver-plugin-test .
```

Run one case directly:

```sh
cd src/compiler
meson compile -j 1 -C build
cd ../..
src/plugin/tests/scripts/run-docker-case.sh \
  src/plugin/tests/fixtures/01_obfuscated \
  pcode-weaver-plugin-test
```

Run through Meson/ApprovalTests:

```sh
cd src/plugin/tests
meson setup build
meson test -C build --print-errorlogs
```
