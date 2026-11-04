#!/bin/sh
# Runs every deterministic test script and stores a transcript per case.
# Usage:  sh tests/run_all.sh          (from the repository root)
#         SEED=7 sh tests/run_all.sh   (different seed)

set -u
BIN=./main
SEED=${SEED:-42}
OUT=tests/out

if [ ! -x "$BIN" ]; then
    echo "Building first..."
    make || exit 1
fi

mkdir -p "$OUT"

for f in tests/in/*.txt; do
    name=$(basename "$f" .txt)
    echo "== $name (seed $SEED) =="
    case "$name" in
        07_debug|08_flare) FLAGS="--debug" ;;
        *)                 FLAGS="" ;;
    esac
    "$BIN" --seed "$SEED" $FLAGS --script "$f" > "$OUT/$name.out" 2>&1
    echo "   exit=$? transcript=$OUT/$name.out"
done

echo "Done. Compare transcripts with the expected results in TESTING_REPORT.md."
