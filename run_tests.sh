#!/bin/sh
# group-73
set -eu

cd "$(dirname "$0")"
make acaderp
passed=0
total=0
actual=
unit_binary=
trap 'rm -f "${actual:-}" "${unit_binary:-}"' EXIT HUP INT TERM
for input in tests/T*.in; do
    expected=${input%.in}.out
    actual=$(mktemp)
    ./acaderp < "$input" > "$actual" 2>/dev/null
    # MSYS2/Windows text mode may emit CRLF; normalize only the platform CR.
    sed -i 's/\r$//' "$actual"
    total=$((total + 1))
    if diff -u "$expected" "$actual"; then
        passed=$((passed + 1))
        printf 'PASS %s\n' "$(basename "$input" .in)"
    else
        printf 'FAIL %s\n' "$(basename "$input" .in)"
        exit 1
    fi
    rm -f "$actual"
done
compiler=${CXX:-g++}
for unit_source in tests/grp_73_material_clone_test.cpp tests/grp_73_model_contract_test.cpp; do
    unit_binary=$(mktemp)
    "$compiler" -std=c++17 -Wall -Wextra -pedantic -Werror -Isrc \
        "$unit_source" src/grp_73_Model.cpp -o "$unit_binary"
    "$unit_binary"
    passed=$((passed + 1))
    total=$((total + 1))
    printf 'PASS %s\n' "$(basename "$unit_source" .cpp | tr '_' '-')"
    rm -f "$unit_binary"
done
printf 'Passed %s/%s tests\n' "$passed" "$total"
