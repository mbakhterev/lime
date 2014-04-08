lmrootnode := $(call nodepath)

.PHONY: lime cleanlime

lime: cstd = c99
lime: lmlib lmtst lmknl
cleanlime: cleanlmlib cleanlmknl cleanlmtst

$(eval $(call headroute,lime,$(lmrootnode)/lib))

include $(lmrootnode)/lib/gnu.mk
include $(lmrootnode)/bin/gnu.mk
include $(lmrootnode)/tst/gnu.mk
