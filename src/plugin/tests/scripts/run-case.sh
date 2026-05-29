#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <case-dir> <compiled-rules-dir>" >&2
    exit 2
fi

case_dir=$1
rules_dir=$2
work_dir=$(mktemp -d)
project_dir="${work_dir}/ghidra-project"
output_file="${work_dir}/main.c"
reoxide_log="${work_dir}/reoxided.log"
ghidra_log="${work_dir}/ghidra.log"
ghidra_timeout_seconds="${GHIDRA_TIMEOUT_SECONDS:-240}"

cleanup() {
    if [[ -n "${reoxide_pid:-}" ]]; then
        kill "${reoxide_pid}" >/dev/null 2>&1 || true
        wait "${reoxide_pid}" >/dev/null 2>&1 || true
    fi
    rm -rf "${work_dir}"
}
trap cleanup EXIT

mkdir -p "${project_dir}"

mapfile -t binaries < <(find "${case_dir}/target" -maxdepth 1 -type f -name '*.elf' | sort)
if [[ ${#binaries[@]} -ne 1 ]]; then
    echo "expected exactly one ELF in ${case_dir}/target, found ${#binaries[@]}" >&2
    exit 1
fi

export PCODE_WEAVER_RULE_DIR="${rules_dir}"
export REOXIDE_CONFIG=/opt/reoxide/reoxide.toml
export REOXIDE_BIND="ipc://${work_dir}/reoxide.sock"
export REOXIDE_HOST="${REOXIDE_BIND}"
export REOXIDE_MANAGE_BIND="ipc://${work_dir}/reoxide-manage.sock"
export REOXIDE_MANAGE_HOST="${REOXIDE_MANAGE_BIND}"

echo "plugin-test: starting reoxided" >&2
reoxided -c "${REOXIDE_CONFIG}" -b "${REOXIDE_BIND}" -m "${REOXIDE_MANAGE_BIND}" \
    >"${reoxide_log}" 2>&1 &
reoxide_pid=$!

reoxide_ready=false
for _ in $(seq 1 50); do
    if [[ -e "${work_dir}/reoxide-manage.sock" ]]; then
        reoxide_ready=true
        break
    fi
    if ! kill -0 "${reoxide_pid}" >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done

if [[ "${reoxide_ready}" != true ]]; then
    echo "reoxided did not become ready" >&2
    cat "${reoxide_log}" >&2
    exit 1
fi

if ! kill -0 "${reoxide_pid}" >/dev/null 2>&1; then
    echo "reoxided exited before Ghidra started" >&2
    cat "${reoxide_log}" >&2
    exit 1
fi

echo "plugin-test: running Ghidra headless decompile for ${binaries[0]}" >&2
if ! timeout "${ghidra_timeout_seconds}s" "${GHIDRA_INSTALL_DIR}/support/analyzeHeadless" \
    "${project_dir}" pcode-weaver-plugin-test \
    -import "${binaries[0]}" \
    -scriptPath /opt/pcode-weaver/scripts \
    -postScript DecompileFunction.java main "${output_file}" \
    -deleteProject \
    >"${ghidra_log}" 2>&1; then
    echo "Ghidra headless run failed or timed out after ${ghidra_timeout_seconds}s" >&2
    echo "----- ghidra.log -----" >&2
    cat "${ghidra_log}" >&2
    echo "----- reoxided.log -----" >&2
    cat "${reoxide_log}" >&2
    exit 1
fi

echo "plugin-test: decompile finished" >&2
cat "${output_file}"
