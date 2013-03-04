#!/bin/bash

cd "$(dirname "$0")"

set -e

filter () {
	sed -ne 's:^\([0-9a-f]\{2,2\}\)\.[0-9]\+\."\(.*\)"$:\1 \2:gp'
}

P=/tmp/lime-tst-atomtab.pipe

test -e "$P" && { echo "exists: $P" 1>&2; false; }

mkfifo "$P"

diff \
	<(filter < "$P" | { time LC_ALL=C sort; } | uniq) \
	<(./gen-atomtab | tee "$P" | { time ./tst-atomtab; } | filter) \
	|| echo "NO!" 1>&2

rm "$P"
