tstbits = $(call bitspath)

tstsrc = tst-rune.c tst-buff.c
tstobj = $(call c2o,$(tstbits),$(tstsrc))

tst: $(T)/tst-rune $(T)/tst-buff $(T)/tst-atomtab

$(T)/tst-rune: $(tstbits)/tst-rune.o $(L)/liblime.a
$(T)/tst-buff: $(tstbits)/tst-buff.o $(L)/liblime.a
$(T)/tst-atomtab: $(tstbits)/tst-atomtab.o $(L)/liblime.a

include $(call o2d,$(tstobj))
