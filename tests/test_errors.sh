#!/usr/bin/env bash

if [ -z "$EXECUTABLE" ]; then
    EXECUTABLE=./build/hololisp 
fi

failed=0

panic () {
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "$1"
  failed=1
}

test_error () {
  echo -n "Testing $1 ... "

  result=$($EXECUTABLE -m -e "$3" 2>&1 > /dev/null)
  error=$?
  if [ "$error" != 1 ]; then
    echo FAILED error code
    panic "$1"
    return
  fi 

  if [ "$result" != "$2" ]; then
    echo FAILED
    panic "'$2' expected, but got '$result'"
    return 
  fi

  echo "ok"
}

for src in tests/errors/*.hl ; do
    out=${src/.hl/.out}
    b=$(basename $src)
    name=${b/.hl/}
    test_error "$name" "$(cat $out)" "$(cat $src)"
done

exit $failed
