lmiobits := $(call bitspath)

lmiosrc = \
	posix.c

lmioinc = \
	$(I)/lime/fs.h

lmioobj = $(call c2o,$(lmiobits),$(lmiosrc))

.PHONY: lmio cleanlmio

lmio: $(L)/liblimeio.a $(lmioinc)

cleanlmio:
	@ rm -r $(lmiobits) \
	&& rm $(lmioinc) \
	&& rm $(L)/liblimeio.a

$(L)/liblimeio.a: $(lmioobj)

-include $(call o2d,$(lmioobj))
