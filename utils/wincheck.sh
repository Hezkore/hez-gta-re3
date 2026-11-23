#!/bin/sh
# wincheck.sh [FILES...]: compile-check source files as the Windows Direct3D 9 build with mingw, run from any checkout
# with no files it checks the .cpp files changed since the last commit
# needs x86_64-w64-mingw32-g++

COMPILER=x86_64-w64-mingw32-g++
TIME_LIMIT=120
# Windows headers the source names in mixed case, while mingw ships them in lower case
MIXED_CASE_HEADERS="DShow.h IntSafe.h Windows.h"

root=$(git rev-parse --show-toplevel) || exit 1
cd "$root" || exit 1

if ! command -v $COMPILER >/dev/null; then
	echo "wincheck: $COMPILER is missing"
	exit 1
fi

files="$*"
if [ -z "$files" ]; then
	files=$(git diff --name-only HEAD -- 'src/*.cpp')
fi
if [ -z "$files" ]; then
	echo "wincheck: no changed .cpp files"
	exit 0
fi

headers=$(mktemp -d)
trap 'rm -rf "$headers"' EXIT
mingw_include=$(echo '#include <windows.h>' | $COMPILER -x c++ -E -H - 2>&1 >/dev/null | grep -m1 'windows.h' | sed 's|^\.* ||; s|/windows.h$||')
for header in $MIXED_CASE_HEADERS; do
	ln -s "$mingw_include/$(echo "$header" | tr 'A-Z' 'a-z')" "$headers/$header"
done

includes="-I$headers"
for dir in src src/*/ src/audio/eax src/audio/oal src/skel/win vendor/librw vendor/openal-soft/include vendor/libsndfile/include vendor/mpg123/include; do
	includes="$includes -I$dir"
done

failed=0
for file in $files; do
	echo "wincheck: $file"
	if ! timeout $TIME_LIMIT $COMPILER -std=c++14 -fsyntax-only -w \
		-DRW_D3D9 -DUSE_D3D9 -DLIBRW -DAUDIO_OAL -DNDEBUG -D_CRT_SECURE_NO_WARNINGS \
		$includes "$file"; then
		failed=1
	fi
done

if [ $failed = 1 ]; then
	echo "wincheck: failed"
	exit 1
fi
echo "wincheck: ok"
