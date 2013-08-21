lmrootnode := $(call nodepath)

cstd = c99

lime: lmlib lmknl lmtst 

$(eval $(call headroute,lime,$(lmrootnode)/lib/lime))

include $(lmrootnode)/lib/lime/gnu.mk
include $(lmrootnode)/bin/knl/gnu.mk
include $(lmrootnode)/tst/gnu.mk
