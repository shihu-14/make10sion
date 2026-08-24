#!/bin/zsh

set -euo pipefail

script_dir=${0:A:h}
repo_root=${script_dir:h}
output_dir=${TMPDIR:-/private/tmp}
test_binary="$output_dir/make10sion-battle-card-interaction-tests"

xcrun clang++ \
	-std=c++20 \
	-Wall \
	-Wextra \
	-Werror \
	"$repo_root/tests/BattleCardInteractionTests.cpp" \
	-o "$test_binary"

"$test_binary"
zsh "$script_dir/run-debug-tooling-tests.sh"
