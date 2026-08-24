#!/bin/zsh

set -euo pipefail

repo_root=${1:-${0:A:h:h}}
input_paths=(
	src
	macos/make10sion.xcodeproj/project.pbxproj
	macos/Info.plist
)

{
	git -C "$repo_root" rev-parse HEAD
	git -C "$repo_root" diff --no-ext-diff --binary HEAD -- "${input_paths[@]}"
	while IFS= read -r file; do
		[[ -z "$file" ]] && continue
		print -r -- "untracked=$file"
		shasum -a 256 "$repo_root/$file"
	done < <(git -C "$repo_root" ls-files --others --exclude-standard -- "${input_paths[@]}")
} | shasum -a 256 | awk '{ print $1 }'
