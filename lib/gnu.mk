lmbits := $(call bitspath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	ref.c		\
	array.c		\
	list.c		\
	keymap.c	\
	node.c		\
	checkpoint.c	\
	atomtab.c	\
	dump.c		\
	dag.c		\
	loaddag.c	\
	exprewrite.c	\
	form.c		\
	area.c		\
	enveval.c	\
	typeeval.c	\
	symeval.c	\
	leval.c		\
	ntheval.c	\
	exeqeval.c	\
	formeval.c	\
	eval.c		\
	areaeval.c	\
	progress.c

lminc = \
	$(I)/lime/construct.h	\
	$(I)/lime/util.h	\
	$(I)/lime/heap.h	\
	$(I)/lime/rune.h

lmobj = $(call c2o,$(lmbits),$(lmsrc))

.PHONY: lmlib cleanlmlib

lmlib: $(L)/liblime.a $(lminc)

cleanlmlib:
	@ rm -r $(lmbits) \
	&& rm -r $(I)/lime \
	&& rm $(L)/liblime.a

$(L)/liblime.a: $(lmobj)

$(lmbits)/util.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

-include $(call o2d,$(lmobj))
