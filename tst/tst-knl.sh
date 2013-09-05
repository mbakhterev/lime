#!/usr/bin/bash

set -e

base="$(dirname "$0")/.."
cd "${base}/bin"

# ./mc-lime-cfe <<<"void fn(void) { - - + (a - b) + d ++; }"
# 	| tee -a /dev/stderr \

code="void fn(void) { a; }"

cat <<<"${code}"
echo

./mc-lime-cfe <<<"${code}"

./mc-lime-cfe <<<"${code}" \
	| ./lime-knl -f "${base}/tst/forms/simple-atom.lk"
