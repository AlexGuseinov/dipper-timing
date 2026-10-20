# Builds the shared sources from common/ and impl/ as one RIOT module.
MODULE = dipper_mod
SRC = dudect.c variants.c dipper.c dipper_fast.c dipper_fast_const.c dipper_ct.c dipper_leaky.c
ifeq ($(SET),placement)
  SRC += dipper_placement.c microbench.c
endif
vpath %.c $(CURDIR)/../../../common $(CURDIR)/../../../impl
include $(RIOTBASE)/Makefile.base
