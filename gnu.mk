lmrootnode := $(call nodepath)

# cstd = c99

.PHONY: lime cleanlime

lime: cstd = c99
lime: lmlib lmknl lmtst 
cleanlime: cleanlmlib cleanlmknl cleanlmtst

$(eval $(call headroute,lime,$(lmrootnode)/lib/lime))

include $(lmrootnode)/lib/lime/gnu.mk
include $(lmrootnode)/bin/knl/gnu.mk
include $(lmrootnode)/tst/gnu.mk
