lmrootnode := $(call nodepath)

.PHONY: lime cleanlime

lime: cstd = c99
lime: lmlib lmio lmknl
cleanlime: cleanlmlib cleanlmio cleanlmknl

limetest: lmtst
cleanlimetest: cleanlmtst

$(eval $(call headroute,lime,$(lmrootnode)/lib/lime))
$(eval $(call headroute,lime,$(lmrootnode)/lib/io))

include $(lmrootnode)/lib/lime/gnu.mk
include $(lmrootnode)/lib/io/gnu.mk
include $(lmrootnode)/bin/knl/gnu.mk
include $(lmrootnode)/tst/gnu.mk
