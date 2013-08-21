lkbits := $(call bitspath)

lksrc = \
	kernel.c

lkobj = $(call c2o, $(lkbits),$(lksrc))

.PHONY: lmknl
lmknl: $(B)/lime-knl

$(B)/lime-knl: $(lkobj) $(L)/liblime.a

$(call o2d,$(lkobj)): cflags += -I $(I)
$(lkobj): cflags += -I $(I)

-include $(call o2d,$(lkobj))
