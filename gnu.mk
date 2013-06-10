lmrootnode := $(call nodepath)

cstd = c99

lime: cflags += -I $(lmrootnode)/lib/
lime: lmtst lmlib 

$(eval $(call headroute,lime,$(lmrootnode)/lib/lime))

include $(lmrootnode)/lib/lime/gnu.mk
include $(lmrootnode)/tst/gnu.mk
