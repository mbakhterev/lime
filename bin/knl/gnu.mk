lkbits := $(call bitspath)

lksrc = \
	kernel.c

lkobj = $(call c2o, $(lkbits),$(lksrc))

.PHONY: lmknl cleanlmknl

lmknl: $(B)/mc-knl

cleanlmknl:
	@ rm -r $(lkbits)/*.o \
	&& rm $(B)/mc-knl

# $(B)/mc-knl: $(lkobj) $(L)/liblime.a

$(B)/mc-knl: lflags += -L $(L) -llime
$(B)/mc-knl: $(lkobj) $(L)/liblime.a

$(lkbits)/kernel.o: cflags += -D_XOPEN_SOURCE

$(call o2d,$(lkobj)): cflags += -I $(I)
$(lkobj): cflags += -I $(I)

-include $(call o2d,$(lkobj))
