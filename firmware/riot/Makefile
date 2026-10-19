# Dipper timing-leakage test, one RIOT application for all Cortex-M boards.
#   make BOARD=iotlab-m3        (also samr21-xpro, nrf52840dk, b-l475e-iot01a, rpi-pico)
#   make BOARD=iotlab-m3 SEED=2 BINDIRBASE=bin-s2      (repeat run, new seed)
APPLICATION = dipper-timing
BOARD ?= iotlab-m3
APPDIR := $(CURDIR)
RIOTBASE ?= $(APPDIR)/RIOT
SEED ?= 1
N ?= 1000000
NRAW ?= 131072
VARIANTS ?= 0x1F
TESTS ?= 3

# same optimisation for every variant
CFLAGS_OPT = -O2
CFLAGS += -ffp-contract=off
CFLAGS += -I$(APPDIR)/../../common -I$(APPDIR)/../../impl -I$(APPDIR)
CFLAGS += -DCFG_SEED=$(SEED) -DCFG_N=$(N) -DCFG_NRAW=$(NRAW) -DCFG_VARIANTS=$(VARIANTS) -DCFG_TESTS=$(TESTS)
CFLAGS += -DTHREAD_STACKSIZE_MAIN=4096
CFLAGS += -DBUILD_INFO=\"riot-$(BOARD)-O2-seed$(SEED)\"

# RAM-limited boards: smaller calibration batch and measurement batch
ifneq (,$(filter samr21-xpro,$(BOARD)))
  CFLAGS += -DCFG_NCAL=2500 -DDUDECT_CAL_MAX=2500 -DDUDECT_BATCH=128
else ifneq (,$(filter iotlab-m3,$(BOARD)))
  CFLAGS += -DCFG_NCAL=8000 -DDUDECT_CAL_MAX=8000 -DDUDECT_BATCH=128
else
  CFLAGS += -DCFG_NCAL=10000 -DDUDECT_CAL_MAX=10000
endif

USEMODULE += dipper_mod ztimer_msec
DIRS += $(APPDIR)/dipper_mod

QUIET ?= 1
include $(RIOTBASE)/Makefile.include
