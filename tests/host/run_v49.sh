#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
flags=(-std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware)
run() { local name=$1; shift; echo "=== $name ==="; g++ "${flags[@]}" "$@" "tests/host/test_${name}.cpp" -o "$build/$name"; "$build/$name"; }
run maze src/firmware/maze_state.cpp
run maze_planner_v48
run nav_laser_policy
run nav_opening
run navigation_v48 -DNAV_HOST_SIM src/firmware/maze_state.cpp src/firmware/navigation.cpp
run motion src/firmware/motion_control.cpp
run giro src/firmware/motion_control.cpp
run partida src/firmware/motion_control.cpp
run centralizacao src/firmware/motion_control.cpp
