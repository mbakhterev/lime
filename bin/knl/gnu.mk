lkbits := $(call bitspath)

lksrc = \
	kernel.c

lkobj = $(call c2o, $(lkbits),$(lksrc))

.PHONY: lmknl cleanlmknl

lmknl: $(B)/lime-knl

cleanlmknl:
	@ rm -r $(lkbits)/*.o \
	&& rm $(B)/lime-knl

$(B)/lime-knl: $(lkobj) $(L)/liblime.a

$(lkbits)/kernel.o: cflags += -D_XOPEN_SOURCE

$(call o2d,$(lkobj)): cflags += -I $(I)
$(lkobj): cflags += -I $(I)

-include $(call o2d,$(lkobj))
