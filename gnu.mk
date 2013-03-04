include toolchain.mk
include general.mk

cstd = c99
cflags += -I lib/

all: tst

include lib/lime/gnu.mk
include tst/gnu.mk
