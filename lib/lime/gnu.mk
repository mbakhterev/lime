lmbits := $(call bitspath)

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
	walkdag.c	\
	evallists.c	\
	context.c	\
	form.c		\
	evalforms.c

lminc = \
	$(I)/lime/construct.h	\
	$(I)/lime/util.h	\
	$(I)/lime/heap.h	\
	$(I)/lime/rune.h

lmobj = $(call c2o,$(lmbits),$(lmsrc))

.PHONY: lmlib
lmlib: $(L)/liblime.a $(lminc)

$(L)/liblime.a: $(lmobj)

$(lmbits)/util.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

-include $(call o2d,$(lmobj))
