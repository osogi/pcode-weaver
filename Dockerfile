ARG PLUGIN_TEST_BASE_IMAGE=pcode-weaver-plugin-test-base:ghidra-12.0_reoxide-0.7.2

FROM quay.io/pypa/manylinux2014 AS plugin-build

ARG REOXIDE_VERSION=0.7.2
ENV VIRTUAL_ENV=/venv
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"
ENV PIP_INDEX_URL=https://pypi.org/simple
ENV PIP_DISABLE_PIP_VERSION_CHECK=1

RUN set -ex;\
    manylinux-interpreters ensure cp311-cp311;\
    /opt/python/cp311-cp311/bin/python -m venv "${VIRTUAL_ENV}"

RUN --mount=type=cache,target=/root/.cache/pip \
    set -ex;\
    pip install --index-url "${PIP_INDEX_URL}" "meson>=1.10.1" "meson-python>=0.17" "ninja>=1.11" "reoxide==${REOXIDE_VERSION}";\
    mkdir -p /root/.config/reoxide;\
    touch /root/.config/reoxide/reoxide.toml

WORKDIR /plugin
COPY src/inc/ ./src/inc/
COPY src/plugin/meson.build ./src/plugin/meson.build
COPY src/plugin/main/ ./src/plugin/main/

RUN set -ex;\
    meson setup build ./src/plugin/ --buildtype release;\
    meson install -C build

FROM ${PLUGIN_TEST_BASE_IMAGE} AS plugin-test

COPY --from=plugin-build /root/.local/share/reoxide/plugins/*.so /opt/reoxide/data/plugins/
COPY src/plugin/tests/scripts/ /opt/pcode-weaver/scripts/
COPY scripts/install-pcodeweaver-action.sh /opt/pcode-weaver/scripts/install-pcodeweaver-action.sh

RUN set -ex;\
    default_reoxide_yaml="$(python3 -c 'from pathlib import Path; import reoxide; print(Path(reoxide.__file__).parent / "data" / "default.yaml")')";\
    cp "${default_reoxide_yaml}" /opt/reoxide/data/current.yaml;\
    sh /opt/pcode-weaver/scripts/install-pcodeweaver-action.sh /opt/reoxide/data/current.yaml;\
    chmod +x /opt/pcode-weaver/scripts/run-case.sh \
        /opt/pcode-weaver/scripts/install-pcodeweaver-action.sh

FROM scratch

COPY --from=plugin-build /root/.local/share/reoxide/plugins/*.so .
