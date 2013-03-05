tstbits = $(call bitspath)

tstsrc = \
	tst-rune.c \
	tst-buff.c \
	tst-loadatom.c \
	tst-loadtoken.c \
	tst-heapsort.c

tstobj = $(call c2o,$(tstbits),$(tstsrc))

lib = $(L)/liblime.a

tst: \
	$(T)/tst-rune \
	$(T)/tst-buff \
	$(T)/tst-heapsort \
	$(T)/tst-loadatom \
	$(T)/tst-loadtoken \
	$(T)/gen-atomtab \
	$(T)/tst-atomtab.sh

$(T)/tst-rune: $(tstbits)/tst-rune.o $(lib)
$(T)/tst-buff: $(tstbits)/tst-buff.o $(lib)
$(T)/tst-heapsort: $(tstbits)/tst-heapsort.o $(lib)

$(T)/tst-loadatom: $(tstbits)/tst-loadatom.o $(lib)
$(T)/tst-loadtoken: $(tstbits)/tst-loadtoken.o $(lib)
$(T)/gen-atomtab: $(tstbits)/gen-atomtab.o $(lib)
$(T)/tst-atomtab.sh: tst/tst-atomtab.sh

include $(call o2d,$(tstobj))
