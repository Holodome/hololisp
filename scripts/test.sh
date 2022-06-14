#!/usr/bin/env bash

unit_test_files=$(find build -type f -name "*.test" | xargs)
test_script_files=$(find tests -type f -name "*.sh" | xargs)

failed_tests_file=/tmp/hololisp_tests_failed.txt
true > "$failed_tests_file"

success_count=0
error_count=0

process_test () {
    executable=$1
    ok_pattern=$2
    fail_pattern=$3

    exec_result=$($executable 2> /dev/null)

    ok_count=$(echo "$exec_result" | grep -ocF "$ok_pattern")
    failed_count=$(echo "$exec_result" | grep -ocF "$fail_pattern")

    if [ "$failed_count" != 0 ]; then
        failed_lines=$(echo "$exec_result" | grep -F "$fail_pattern")
        echo -e "\e[1;93m>>>\e[1;0m $executable" >> "$failed_tests_file"
        echo "$failed_lines" >> "$failed_tests_file"
    fi

    success_count=$((success_count + ok_count))
    error_count=$((error_count + failed_count))
}

for unit_test in $unit_test_files ;
do process_test "$unit_test" '[ OK ]' "[ FAILED ]"
done

for script in $test_script_files ;
do process_test "$script" "ok" "[ERROR]"
done

total_test_count=$((success_count + error_count))
echo -e "\e[1;32mTESTS PASSED\e[1;0m $success_count/$total_test_count"

exit_code=0
if [ "$error_count" != 0 ]; then
    echo -e "\e[1;31m$error_count Failed\e[0m"
    cat "$failed_tests_file"
    exit_code=1
fi

exit "$exit_code"
