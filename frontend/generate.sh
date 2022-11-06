#!/usr/bin/env bash

set -e

if [[ $(basename $(pwd)) != frontend ]]; then
    echo script must be executed from frontend dir
    exit 1
fi

rm -rf generated
mkdir -p generated

pushd ..
make wasm
popd

python3 - > generated/examples.js <<EOF
files = ["../examples/mazegen.hl", "../examples/quicksort.hl", "../examples/game_of_life.hl", "../examples/hello.hl"]
names = ["maze generator", "quicksort", "game of life", "hello world"]

print("export const Examples = {")
for (file, name) in zip(files, names):
    print(f"\"{name}\": \`{open(file).read()}\`,")
print("};")
EOF

cp ../build/hololisp.js generated
cp ../build/hololisp.wasm generated

echo DONE
