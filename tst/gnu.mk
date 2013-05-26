tstbits = $(call bitspath)

tstsrc = \
	tst-rune.c	\
	tst-array.c	\
	tst-loadatom.c	\
	tst-loadtoken.c	\
	tst-heapsort.c	\
	tst-list.c	\
	tst-loaddag.c

tstobj = $(call c2o,$(tstbits),$(tstsrc))

lib = $(L)/liblime.a

tst: cflags += -I $(I)
tst: \
	$(T)/tst-rune		\
	$(T)/tst-array		\
	$(T)/tst-heapsort	\
	$(T)/tst-loadatom	\
	$(T)/tst-loadtoken	\
	$(T)/gen-atomtab	\
	$(T)/tst-atomtab.sh	\
	$(T)/tst-list		\
	$(T)/tst-loaddag	\
	$(T)/tst-gcnodes

$(T)/tst-rune: $(tstbits)/tst-rune.o $(lib)
$(T)/tst-array: $(tstbits)/tst-array.o $(lib)
$(T)/tst-heapsort: $(tstbits)/tst-heapsort.o $(lib)

$(T)/tst-loadatom: $(tstbits)/tst-loadatom.o $(lib)
$(T)/tst-loadtoken: $(tstbits)/tst-loadtoken.o $(lib)
$(T)/gen-atomtab: $(tstbits)/gen-atomtab.o $(lib)
$(T)/tst-atomtab.sh: tst/tst-atomtab.sh

$(T)/tst-list: $(tstbits)/tst-list.o $(lib)
$(T)/tst-loaddag: $(tstbits)/tst-loaddag.o $(lib)
$(T)/tst-gcnodes: $(tstbits)/tst-gcnodes.o $(lib)

include $(call o2d,$(tstobj))
