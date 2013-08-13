lmtstbits = $(call bitspath)
lmtstnode = $(call nodepath)

lmtstsrc = \
	tst-rune.c	\
	tst-array.c	\
	tst-loadatom.c	\
	tst-loadtoken.c	\
	tst-heapsort.c	\
	tst-list.c	\
	tst-loaddag.c	\
	tst-gcnodes.c	\
	tst-forkdag.c	\
	tst-evallists.c

tstobj = $(call c2o,$(lmtstbits),$(lmtstsrc))

lmlib = $(L)/liblime.a

# lmtst: cflags += -I $(I)

.PHONY: lmtst

lmtst: \
	$(T)/tst-rune		\
	$(T)/tst-array		\
	$(T)/tst-heapsort	\
	$(T)/tst-loadatom	\
	$(T)/tst-loadtoken	\
	$(T)/gen-atomtab	\
	$(T)/tst-atomtab.sh	\
	$(T)/tst-list		\
	$(T)/tst-loaddag	\
	$(T)/tst-gcnodes	\
	$(T)/tst-forkdag	\
	$(T)/tst-evallists	\
	$(T)/tst-evalforms

$(T)/tst-rune: $(lmtstbits)/tst-rune.o $(lmlib)
$(T)/tst-array: $(lmtstbits)/tst-array.o $(lmlib)
$(T)/tst-heapsort: $(lmtstbits)/tst-heapsort.o $(lmlib)

$(T)/tst-loadatom: $(lmtstbits)/tst-loadatom.o $(lmlib)
$(T)/tst-loadtoken: $(lmtstbits)/tst-loadtoken.o $(lmlib)
$(T)/gen-atomtab: $(lmtstbits)/gen-atomtab.o $(lmlib)
$(T)/tst-atomtab.sh: $(lmtstnode)/tst-atomtab.sh

$(T)/tst-list: $(lmtstbits)/tst-list.o $(lmlib)
$(T)/tst-loaddag: $(lmtstbits)/tst-loaddag.o $(lmlib)
$(T)/tst-gcnodes: $(lmtstbits)/tst-gcnodes.o $(lmlib)
$(T)/tst-forkdag: $(lmtstbits)/tst-forkdag.o $(lmlib)
$(T)/tst-evallists: $(lmtstbits)/tst-evallists.o $(lmlib)
$(T)/tst-evalforms: $(lmtstbits)/tst-evalforms.o $(lmlib)

$(call o2d,$(tstobj)): cflags += -I $(lmrootnode)/lib
-include $(call o2d,$(tstobj))
