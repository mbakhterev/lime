lmtstbits := $(call bitspath)
lmtstnode := $(call nodepath)

lmtstsrc = \
	tst-rune.c	\
	tst-array.c	\
	gen-atomtab.c	\
	tst-loadatom.c	\
	tst-loadtoken.c	\
	tst-heapsort.c	\
	tst-list.c	\
	tst-loaddag.c	\
	tst-gcnodes.c	\
	tst-forkdag.c	\
	tst-evallists.c \
	tst-evalforms.c

lmtstobj = $(call c2o,$(lmtstbits),$(lmtstsrc))

# Targets
lmtstbin = \
	$(T)/tst-rune		\
	$(T)/tst-array		\
	$(T)/tst-heapsort	\
	$(T)/tst-loadatom	\
	$(T)/tst-loadtoken	\
	$(T)/gen-atomtab	\
	$(T)/tst-atomtab.sh	\

# 	$(T)/tst-list		\
# 	$(T)/tst-loaddag	\
# 	$(T)/tst-gcnodes	\
# 	$(T)/tst-forkdag	\
# 	$(T)/tst-evallists	\
# 	$(T)/tst-evalforms	\
# 	$(T)/tst-knl.sh

lmlib = $(L)/liblime.a

.PHONY: lmtst cleanlmtst

lmtst: $(lmtstbin)

cleanlmtst:
	@ rm -r $(lmtstbits) \
	&& rm $(lmtstbin)

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

lmtstforms = \
	$(T)/forms/simple-atom.lk

$(T)/tst-knl.sh: $(lmtstnode)/tst-knl.sh $(lmtstforms)

# Правило подхвата тестовых форм

$(T)/%.lk: $(lmtstnode)/%.lk
	@ $(echo) '\tform\t$@' \
	&& $(call mkpath,$(BDIR),$(@D)) \
	&& install -m 755 $< $@

$(lmtstobj) $(call o2d,$(lmtstobj)): cflags += -I $(I)
$(call o2d,$(lmtstobj)): $(lminc)

-include $(call o2d,$(lmtstobj))
