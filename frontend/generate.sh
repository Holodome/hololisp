#!/usr/bin/env bash

set -e

if [[ $(basename $(pwd)) != frontend ]]; then
    echo script must be executed from frontend dir
    exit 1
fi

# Generate webassembly code

pushd ..
make wasm
popd

# copy webassembly output

rm -rf generated
mkdir -p generated
cp ../build/hololisp.js generated
cp ../build/hololisp.wasm generated

echo DONE
