#!/usr/bin/env bash

set -x

# shellcheck disable=SC2046
clang-format -i --style=file:.clang-format $(find . -type f -name "*.c" -o -name "*.h")
