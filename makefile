root := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(root)/general.mk

.PHONY: all lime

all: lime
clean: cleanlime

$(call checkdefs,\
	cc lnk, toolchain should define: cc lnk)

cstd = c99

include $(root)/gnu.mk
