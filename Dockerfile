FROM quay.io/pypa/manylinux2014 AS build

ARG REOXIDE_VERSION=0.7.2

RUN set -ex;\
    manylinux-interpreters ensure cp311-cp311;\
    /opt/python/cp311-cp311/bin/python -m venv /venv;\
    . /venv/bin/activate;\
    pip install "meson>=1.10.1" "meson-python>=0.17" "ninja>=1.11" "reoxide==${REOXIDE_VERSION}";\
    deactivate;\
    mkdir -p /root/.config/reoxide;\
    touch /root/.config/reoxide/reoxide.toml

WORKDIR /plugin
COPY . .

RUN set -ex;\
    . /venv/bin/activate;\
    meson setup build --buildtype release;\
    meson install -C build

FROM scratch

COPY --from=build /root/.local/share/reoxide/plugins/*.so .
