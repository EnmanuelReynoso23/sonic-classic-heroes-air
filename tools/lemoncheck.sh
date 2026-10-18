#!/bin/bash
# Checks the mod with the same lemonscript compiler as Sonic 3 A.I.R., without needing the ROM.
# Runs on Linux / WSL (needs g++, cmake, zlib).
#
#   tools/lemoncheck.sh build   -> fetch S3AIR sources (once) and build the checker
#   tools/lemoncheck.sh check   -> compile the base game scripts + this mod
#   tools/lemoncheck.sh test    -> compile, then run the simulation tests in tests/heroes_test.lemon
#
# Environment variables (optional):
#   S3AIR_TAG      game version to check against (default: v26.03.28.0-stable)
#   S3AIR_SCRIPTS  folder with the game's script sources ("bonus/sonic3air_dev/scripts" of a game installation);
#                  by default taken from the S3AIR repository at S3AIR_TAG
set -e
MOD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
WORK_DIR="${LEMONCHECK_DIR:-$HOME/.cache/sonic-classic-heroes-air}"
TAG="${S3AIR_TAG:-v26.03.28.0-stable}"
SRC="$WORK_DIR/sonic3air-$TAG"
BIN="$WORK_DIR/build-$TAG/lemoncheck"
SCRIPTS="${S3AIR_SCRIPTS:-$SRC/Oxygen/sonic3air/scripts}"

build() {
	mkdir -p "$WORK_DIR"
	if [ ! -d "$SRC" ]; then
		git clone --depth 1 --branch "$TAG" --filter=blob:none --sparse https://github.com/Eukaryot/sonic3air.git "$SRC"
		git -C "$SRC" sparse-checkout set librmx Oxygen/lemonscript Oxygen/sonic3air/scripts
	fi
	cmake -S "$MOD_DIR/tools/lemoncheck" -B "$WORK_DIR/build-$TAG" -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
		-DS3AIR_REPO="$SRC" > /dev/null
	cmake --build "$WORK_DIR/build-$TAG" -j "$(nproc)" 2>&1 | grep -E "error|Built target" || true
}

export ASAN_OPTIONS=detect_leaks=0
case "${1:-check}" in
	build)
		build ;;
	check)
		[ -x "$BIN" ] || build
		cd "$SCRIPTS" && "$BIN" main.lemon _reference/cpp_core_functions.lemon "$MOD_DIR/scripts/main.lemon" ;;
	test)
		[ -x "$BIN" ] || build
		cd "$SCRIPTS" && "$BIN" main.lemon _reference/cpp_core_functions.lemon "$MOD_DIR/scripts/main.lemon" "$MOD_DIR/tests/heroes_test.lemon" --run Test.run ;;
	*)
		echo "Usage: $0 build|check|test"; exit 2 ;;
esac
