#!/usr/bin/env bash

cargo install bindgen

bindgen ./libsettings_wrapper.h -o src/bindings.rs -- \
    -I../../include \
    -I../../third_party/libswiftnav/include \
    -I../../third_party/libsbp/c/include
