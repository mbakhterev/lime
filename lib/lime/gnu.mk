lmbits = $(call bitspath)

lmsrc = rune.c atomtab.c buffer.c heapsort.c
lmobj = $(call c2o,$(bitspath),$(lmsrc))

$(L)/liblime.a: $(lmobj)

include $(call o2d,$(lmobj))
