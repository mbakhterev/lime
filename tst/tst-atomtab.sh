#!/bin/bash

cd "$(dirname "$0")"

set -e

filter () {
	sed -ne 's:^\([0-9a-f]\{2,2\}\)\.[0-9]\+\."\(.*\)"$:\1 \2:gp'
}

P=/tmp/lime-tst-atomtab.pipe

test -e "$P" && { echo "exists: $P" 1>&2; false; }

trap "rm '$P'" EXIT
mkfifo "$P"

diff \
	<(filter < "$P" | LC_ALL=C sort | uniq) \
	<(./gen-atomtab | tee "$P" | ./tst-loadatom | filter) \
	|| { echo "loadatom: NOT PASSED" 1>&2; false; }

rm "$P"
mkfifo "$P"

diff \
	<(LC_ALL=C sort < "$P" | uniq) \
	<(./gen-atomtab | tee "$P" | ./tst-loadtoken) \
	|| { echo "loadtoken: NOT PASSED" 1>&2; false; }


