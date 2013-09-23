lmbits := $(call bitspath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	array.c		\
	atomtab.c	\
	list.c		\
	environment.c	\
	loaddag.c	\
	dump.c		\
	dag.c		\
	walkdag.c	\
	evallists.c	\
	context.c	\
	form.c		\
	evalforms.c	\
	progress.c	\
	checkpoint.c

#	uimap.c		\
#	ptrmap.c	\

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
	&& rm $(L)/liblime.a \
	&& rm -r $(I)/lime

$(L)/liblime.a: $(lmobj)

$(lmbits)/util.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

-include $(call o2d,$(lmobj))
