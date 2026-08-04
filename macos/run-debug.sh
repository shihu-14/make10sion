#!/bin/zsh

set -euo pipefail

script_dir=${0:A:h}
repo_root=${script_dir:h}
app_path="$repo_root/macos/App/make10sion.app"
binary_path="$app_path/Contents/MacOS/make10sion"
manifest_path="$repo_root/macos/App/make10sion-build-provenance.txt"
expected_sha=$(git -C "$repo_root" rev-parse HEAD)

if [[ ! -f "$binary_path" || ! -f "$manifest_path" ]]; then
	echo "No verified build is available. Run 'Siv3D: Build macOS' first." >&2
	exit 1
fi

actual_root=$(sed -n 's/^repo_root=//p' "$manifest_path")
actual_sha=$(sed -n 's/^git_sha=//p' "$manifest_path")
actual_binary_path=$(sed -n 's/^binary_path=//p' "$manifest_path")
expected_binary_sha=$(sed -n 's/^binary_sha256=//p' "$manifest_path")
actual_binary_sha=$(shasum -a 256 "$binary_path" | awk '{ print $1 }')
if [[ "$actual_root" != "$repo_root" || "$actual_sha" != "$expected_sha"
	|| "$actual_binary_path" != "$binary_path" || "$actual_binary_sha" != "$expected_binary_sha" ]]; then
	echo "Refusing to run a stale or foreign make10sion build." >&2
	echo "expected repo=$repo_root sha=$expected_sha binary=$binary_path hash=$expected_binary_sha" >&2
	echo "actual repo=$actual_root sha=$actual_sha binary=$actual_binary_path hash=$actual_binary_sha" >&2
	exit 1
fi

cat "$manifest_path"
if [[ "${1:-}" == "--check" ]]; then
	exit 0
fi
if (( $# != 0 )); then
	echo "Usage: $0 [--check]" >&2
	exit 2
fi
cd "$repo_root/macos/App"
exec "$binary_path"
