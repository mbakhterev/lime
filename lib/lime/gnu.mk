lmbits = $(call bitspath)

lmsrc = \
	rune.c		\
	atomtab.c	\
	array.c		\
	heapsort.c	\
	util.c		\
	list.c		\
	environment.c

lmobj = $(call c2o,$(bitspath),$(lmsrc))

$(L)/liblime.a: $(lmobj)

$(lmbits)/list.o: cflags += -D_XOPEN_SOURCE=700

include $(call o2d,$(lmobj))
