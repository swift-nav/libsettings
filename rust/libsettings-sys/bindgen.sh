#!/usr/bin/env bash

cargo install bindgen

bindgen ./libsettings_wrapper.c -o src/bindings.rs -- \
    -I../../include \
    -I../../third_party/libsbp/c/include
