FROM quay.io/pypa/manylinux2014 AS build

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
COPY . .

RUN set -ex;\
    meson setup build ./src/plugin/ --buildtype release;\
    meson install -C build

FROM scratch

COPY --from=build /root/.local/share/reoxide/plugins/*.so .
