lmbits = $(call bitspath)

lmsrc = \
	rune.c		\
	util.c		\
	heap.c		\
	array.c		\
	atomtab.c	\
	list.c		\
	environment.c	\
	uimap.c

lmobj = $(call c2o,$(bitspath),$(lmsrc))

$(L)/liblime.a: $(lmobj)

$(lmbits)/list.o: cflags += -D_XOPEN_SOURCE=700

include $(call o2d,$(lmobj))
