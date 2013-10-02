lmbits := $(call bitspath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	ref.c		\
	array.c		\
	list.c		\
	keytab.c	\
	checkpoint.c	\
	atomtab.c	\

# 	environment.c	\
# 	loaddag.c	\
# 	dump.c		\
# 	dag.c		\
# 	walkdag.c	\
# 	evallists.c	\
# 	context.c	\
# 	form.c		\
# 	evalforms.c	\
# 	progress.c	\
	
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
