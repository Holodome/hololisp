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

  error=$($EXECUTABLE -m -e "$3" 2>&1 > /dev/null)
  if [ -z "$error" ]; then 
    echo FAILED error code
    panic "$1"
    return
  fi 

  result=$(($EXECUTABLE -m -e "$3" 1> /dev/null) 2>&1)
  if [ "$result" != "$2" ]; then 
    echo FAILED
    panic "'$2' expected, but got '$result'"
    return 
  fi

  echo "ok"
}

test_error "closing paren" "$(cat tests/errors/closing_paren.out)" "$(cat tests/errors/closing_paren.hl)"

exit $failed
