#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "usage: $0 <case-dir> [docker-image]" >&2
    exit 2
fi

case_dir=$(realpath "$1")
image="${2:-${PCODE_WEAVER_PLUGIN_TEST_IMAGE:-pcode-weaver-plugin-test:latest}}"
repo_root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
compiler="${repo_root}/src/compiler/build/bin/compiler.elf"
work_dir=$(mktemp -d)
rules_dir="${work_dir}/compiled-rules"

cleanup() {
    rm -rf "${work_dir}"
}
trap cleanup EXIT

if [[ ! -x "${compiler}" ]]; then
    echo "compiler not found or not executable: ${compiler}" >&2
    echo "build it first from src/compiler with: meson compile -j 1 -C build" >&2
    exit 1
fi

mapfile -t rules < <(find "${case_dir}/rules" -maxdepth 1 -type f | sort)
if [[ ${#rules[@]} -eq 0 ]]; then
    echo "no rules found in ${case_dir}/rules" >&2
    exit 1
fi

mkdir -p "${rules_dir}"
"${compiler}" -d "${rules_dir}" "${rules[@]}" >&2

docker run --rm \
    --volume "${case_dir}:/case:ro" \
    --volume "${rules_dir}:/compiled-rules:ro" \
    "${image}" \
    /opt/pcode-weaver/scripts/run-case.sh /case /compiled-rules
