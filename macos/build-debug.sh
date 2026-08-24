#!/bin/zsh

set -euo pipefail

script_dir=${0:A:h}
repo_root=${script_dir:h}
head_sha=$(git -C "$repo_root" rev-parse HEAD)
source_fingerprint=$(zsh "$script_dir/build-input-fingerprint.sh" "$repo_root")
app_path="$repo_root/macos/App/make10sion.app"
binary_path="$app_path/Contents/MacOS/make10sion"
manifest_path="$repo_root/macos/App/make10sion-build-provenance.txt"
build_root=$(mktemp -d "${TMPDIR:-/private/tmp}/make10sion-debug-build.XXXXXX")
built_app="$build_root/app/make10sion.app"
previous_app="$build_root/previous-make10sion.app"
installed=false

cleanup() {
	if [[ "$installed" != true && -d "$previous_app" && ! -e "$app_path" ]]; then
		mv "$previous_app" "$app_path"
	fi
	rm -rf -- "$build_root"
}
trap cleanup EXIT

echo "Building make10sion from $repo_root at $head_sha ($source_fingerprint)"
xcodebuild \
	-project "$repo_root/macos/make10sion.xcodeproj" \
	-target make10sion \
	-configuration Debug \
	ARCHS=x86_64 \
	ONLY_ACTIVE_ARCH=YES \
	OBJROOT="$build_root/obj" \
	SYMROOT="$build_root/sym" \
	CONFIGURATION_BUILD_DIR="$build_root/app" \
	clean build

completed_source_fingerprint=$(zsh "$script_dir/build-input-fingerprint.sh" "$repo_root")
if [[ "$completed_source_fingerprint" != "$source_fingerprint" ]]; then
	echo "Build inputs changed while xcodebuild was running. Refusing to install this build." >&2
	exit 1
fi

if [[ -d "$app_path" ]]; then
	mv "$app_path" "$previous_app"
fi
ditto "$built_app" "$app_path"
installed=true

binary_sha=$(shasum -a 256 "$binary_path" | awk '{ print $1 }')
binary_uuid=$(dwarfdump --uuid "$binary_path" | sed -n '1p')
binary_mtime=$(stat -f '%Sm' -t '%Y-%m-%dT%H:%M:%S%z' "$binary_path")

{
	echo "repo_root=$repo_root"
	echo "git_sha=$head_sha"
	echo "source_fingerprint=$source_fingerprint"
	echo "binary_path=$binary_path"
	echo "binary_mtime=$binary_mtime"
	echo "binary_sha256=$binary_sha"
	echo "binary_uuid=$binary_uuid"
} > "$manifest_path"

cat "$manifest_path"
