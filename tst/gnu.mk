tstbits = $(call bitspath)

tstsrc = tst-rune.c
tstobj = $(call c2o,$(tstbits),$(tstsrc))

tst: $(T)/tst-rune

$(T)/tst-rune: $(tstbits)/tst-rune.o $(L)/liblime.a

include $(call o2d,$(tstobj))
