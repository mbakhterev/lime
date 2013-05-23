lmbits = $(call bitspath)

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
	loadrawdag.c	\
	loadcontext.c	\
	dag.c		\
	eval.c

lminc = \
	$(I)/lime/construct.h	\
	$(I)/lime/util.h

lmobj = $(call c2o,$(lmbits),$(lmsrc))

liblime: $(L)/liblime.a $(lminc)

$(L)/liblime.a: $(lmobj)

$(lmbits)/list.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)
$(lmbits)/util.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

include $(call o2d,$(lmobj))
