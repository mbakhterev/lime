#!/usr/bin/bash

set -e

base="$(dirname "$0")/.."
cd "${base}/bin"

# ./mc-lime-cfe <<<"void fn(void) { - - + (a - b) + d ++; }"

./mc-lime-cfe <<<"void fn(void) { a; }" \
	| tee -a /dev/stderr \
	| ./lime-knl -f "${base}/tst/forms/simple-atom.lk"
