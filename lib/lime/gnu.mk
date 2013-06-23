lmbits := $(call bitspath)
# lmnode := $(call nodepath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	array.c		\
	atomtab.c	\
	list.c		\
	environment.c	\
	uimap.c		\
	ptrmap.c	\
	loaddag.c	\
	dumpdag.c	\
	dag.c		\
	eval.c

#	ldcontext.c	\


lminc = \
	$(I)/lime/construct.h	\
	$(I)/lime/util.h	\
	$(I)/lime/heap.h

lmobj = $(call c2o,$(lmbits),$(lmsrc))

.PHONY: lmlib
lmlib: $(L)/liblime.a $(lminc)

$(L)/liblime.a: $(lmobj)

# $(lmbits)/list.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)
$(lmbits)/util.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

# $(eval $(call headroute,lime,$(lmnode)))

include $(call o2d,$(lmobj))
