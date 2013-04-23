lmbits = $(call bitspath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	array.c		\
	atomtab.c	\
	node.c		\
	list.c		\
	environment.c	\
	uimap.c		\
	loadrawdag.c

lmobj = $(call c2o,$(bitspath),$(lmsrc))

$(L)/liblime.a: $(lmobj)

$(lmbits)/list.o: cflags += -D_XOPEN_SOURCE=700 $(strictfix)

include $(call o2d,$(lmobj))
