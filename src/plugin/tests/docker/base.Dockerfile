FROM eclipse-temurin:21-jdk-jammy AS ghidra-download

ARG GHIDRA_VERSION=12.0
ARG GHIDRA_DATE=20251205

ENV DEBIAN_FRONTEND=noninteractive
ENV GHIDRA_INSTALL_DIR=/opt/ghidra

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    set -ex;\
    apt-get update;\
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        unzip;\
    curl -L \
        "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_${GHIDRA_VERSION}_build/ghidra_${GHIDRA_VERSION}_PUBLIC_${GHIDRA_DATE}.zip" \
        -o /tmp/ghidra.zip;\
    unzip -q /tmp/ghidra.zip -d /opt;\
    mv "/opt/ghidra_${GHIDRA_VERSION}_PUBLIC" "${GHIDRA_INSTALL_DIR}";\
    rm /tmp/ghidra.zip

FROM eclipse-temurin:21-jdk-jammy

ARG REOXIDE_VERSION=0.7.2

ENV DEBIAN_FRONTEND=noninteractive
ENV GHIDRA_INSTALL_DIR=/opt/ghidra
ENV VIRTUAL_ENV=/opt/reoxide/reoxide-venv
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"
ENV PIP_DISABLE_PIP_VERSION_CHECK=1

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    set -ex;\
    apt-get update;\
    apt-get install -y --no-install-recommends \
        ca-certificates \
        python3 \
        python3-venv;\
    python3 -m venv "${VIRTUAL_ENV}"

RUN --mount=type=cache,target=/root/.cache/pip \
    set -ex;\
    pip install "reoxide==${REOXIDE_VERSION}"

COPY --from=ghidra-download /opt/ghidra /opt/ghidra

RUN set -ex;\
    mkdir -p /opt/reoxide/data/plugins;\
    printf '%s\n' \
        'data-directory = "/opt/reoxide/data"' \
        '' \
        '[[ghidra-install]]' \
        'enabled = true' \
        "root-dir = \"${GHIDRA_INSTALL_DIR}\"" \
        > /opt/reoxide/reoxide.toml;\
    reoxide -c /opt/reoxide/reoxide.toml link-ghidra
