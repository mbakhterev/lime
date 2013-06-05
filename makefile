root := $(patsubst %/,%,$(dir $(firstword $(MAKEFILE_LIST))))

include $(root)/toolchain.mk
include $(root)/general.mk

$(call checkdefs,\
	cc lnk, toolchain should define: cc lnk)

cstd = c99

include $(root)/gnu.mk
