#!/usr/bin/bash

set -e

base="$(dirname "$0")/.."
cd "${base}/bin"

# ./mc-lime-cfe <<<"void fn(void) { - - + (a - b) + d ++; }"
# 	| tee -a /dev/stderr \

./mc-lime-cfe <<<"void fn(void) { a; }" \
	| ./lime-knl -f "${base}/tst/forms/simple-atom.lk"
