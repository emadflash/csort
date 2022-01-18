#!/usr/bin/env bash
### Commands
###
###
### Builds And Run release build
### $ ./build.sh 
###
### Builds debug build
### $ ./build.sh debug
###
### Builds tests with debug flags
### $ ./build.sh  debug_test
###
### Builds and Runs tests with release flags
### $ ./build.sh test

bin="csort"
build_debug_dir="build_debug"
build_test_dir="build_test"
build_release_dir="build"

if [[ "$1" == "debug" ]]; then
    [[ ! -d "${build_debug_dir}" ]] && cmake -B "${build_debug_dir}" -DCMAKE_BUILD_TYPE=Debug
    make -C "${build_debug_dir}" && gdb -q "${build_debug_dir}/${bin}"
elif [[ "$1" == "test" ]]; then
    [[ ! -d "${build_test_dir}" ]] && cmake -B "${build_test_dir}" -DBUILD_TEST=ON
    make -C "${build_test_dir}" && "${build_test_dir}/test"
elif [[ "$1" == "debug_test" ]]; then
    [[ ! -d "${build_test_dir}" ]] && cmake -B "${build_test_dir}" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TEST=ON
    make -C "${build_test_dir}" && gdb -q "${build_test_dir}/test"
elif [[ "$1" == "run_debug" ]]; then
    make -C "${build_debug_dir}" && "${build_debug_dir}/${bin}" "${@}"
else
    [[ ! -d "${build_release_dir}" ]] && cmake -B "${build_release_dir}" -DCMAKE_BUILD_TYPE=Release
    make -C "${build_release_dir}" && "${build_release_dir}/${bin}" "${@}"
fi
