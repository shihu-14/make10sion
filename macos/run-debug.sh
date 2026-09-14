#!/bin/zsh

set -euo pipefail

script_dir=${0:A:h}
repo_root=${script_dir:h}
app_path="$repo_root/macos/App/make10sion.app"
binary_path="$app_path/Contents/MacOS/make10sion"
manifest_path="$repo_root/macos/App/make10sion-build-provenance.txt"
expected_sha=$(git -C "$repo_root" rev-parse HEAD)
expected_source_fingerprint=$(zsh "$script_dir/build-input-fingerprint.sh" "$repo_root")

if [[ ! -f "$binary_path" || ! -f "$manifest_path" ]]; then
	echo "No verified build is available. Run 'Siv3D: Build macOS' first." >&2
	exit 1
fi

actual_root=$(sed -n 's/^repo_root=//p' "$manifest_path")
actual_sha=$(sed -n 's/^git_sha=//p' "$manifest_path")
actual_source_fingerprint=$(sed -n 's/^source_fingerprint=//p' "$manifest_path")
actual_binary_path=$(sed -n 's/^binary_path=//p' "$manifest_path")
expected_binary_sha=$(sed -n 's/^binary_sha256=//p' "$manifest_path")
actual_binary_sha=$(shasum -a 256 "$binary_path" | awk '{ print $1 }')
if [[ "$actual_root" != "$repo_root" || "$actual_sha" != "$expected_sha"
	|| "$actual_source_fingerprint" != "$expected_source_fingerprint"
	|| "$actual_binary_path" != "$binary_path" || "$actual_binary_sha" != "$expected_binary_sha" ]]; then
	echo "Refusing to run a stale or foreign make10sion build." >&2
	echo "expected repo=$repo_root sha=$expected_sha source=$expected_source_fingerprint binary=$binary_path hash=$expected_binary_sha" >&2
	echo "actual repo=$actual_root sha=$actual_sha source=$actual_source_fingerprint binary=$actual_binary_path hash=$actual_binary_sha" >&2
	exit 1
fi

cat "$manifest_path"
run_args=()
if (( $# == 1 )); then
	case "$1" in
		--check)
			exit 0
			;;
		--debug-midgame)
			run_args+=("--debug-midgame")
			;;
		*)
			echo "Usage: $0 [--check|--debug-midgame]" >&2
			exit 2
			;;
	esac
elif (( $# != 0 )); then
	echo "Usage: $0 [--check|--debug-midgame]" >&2
	exit 2
fi
cd "$repo_root/macos/App"
exec "$binary_path" "${run_args[@]}"
