#!/usr/bin/env bash

set -x

clang-format -i --style=file:.clang-format $(find . -type f -name "*.c" -o -name "*.h")
