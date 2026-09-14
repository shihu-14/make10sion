#!/bin/zsh

set -euo pipefail

script_dir=${0:A:h}
repo_root=${script_dir:h}

python3 - "$repo_root/.vscode/tasks.json" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as source:
    tasks = json.load(source)["tasks"]

midgame = next(task for task in tasks if task["label"] == "Siv3D: Run Midgame Debug")
assert midgame.get("dependsOrder") == "sequence"
assert midgame.get("dependsOn") == ["Siv3D: Build macOS"]
PY

fingerprint_script="$repo_root/macos/build-input-fingerprint.sh"
test_repo=$(mktemp -d "${TMPDIR:-/private/tmp}/make10sion-fingerprint-test.XXXXXX")
trap 'rm -rf -- "$test_repo"' EXIT

git -C "$test_repo" init -q
mkdir -p "$test_repo/src" "$test_repo/macos/make10sion.xcodeproj"
print 'int main() {}' > "$test_repo/src/Main.cpp"
print '// project' > "$test_repo/macos/make10sion.xcodeproj/project.pbxproj"
print '<?xml version="1.0"?>' > "$test_repo/macos/Info.plist"
git -C "$test_repo" add src macos
git -C "$test_repo" -c user.name=Test -c user.email=test@example.com commit -q -m baseline

baseline=$(zsh "$fingerprint_script" "$test_repo")
print '// first change' >> "$test_repo/src/Main.cpp"
first_change=$(zsh "$fingerprint_script" "$test_repo")
print '// second change' >> "$test_repo/src/Main.cpp"
second_change=$(zsh "$fingerprint_script" "$test_repo")
print '// untracked' > "$test_repo/src/New.cpp"
with_untracked=$(zsh "$fingerprint_script" "$test_repo")
print '// changed again' >> "$test_repo/src/New.cpp"
changed_untracked=$(zsh "$fingerprint_script" "$test_repo")

if [[ "$baseline" == "$first_change"
	|| "$first_change" == "$second_change"
	|| "$second_change" == "$with_untracked"
	|| "$with_untracked" == "$changed_untracked" ]]; then
	echo "Build input fingerprint did not change with source content." >&2
	exit 1
fi

echo "All debug tooling tests passed"
