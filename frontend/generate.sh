#!/usr/bin/env bash

set -e

if [[ $(basename $(pwd)) != frontend ]]; then
    echo script must be executed from frontend dir
    exit 1
fi

# Generate webassembly code

pushd ..
make wasm

pushd book
mdbook build
popd
popd

# copy webassembly output
# copy book

mkdir -p generated
cp ../build/hololisp.js generated
cp ../build/hololisp.wasm generated
cp -r ../book/book generated

echo DONE
