# Host build: make            (native)
#             make CC=aarch64-linux-gnu-gcc STATIC=1   (cross, static)
CC      ?= cc
OPT     ?= -O2
CFLAGS  ?= $(OPT) -Wall -Wextra -ffp-contract=off
EXTRA   ?=
ROOT    := ../..
SRC     := main.c $(ROOT)/common/dudect.c $(ROOT)/impl/variants.c \
           $(ROOT)/impl/dipper.c $(ROOT)/impl/dipper_fast.c $(ROOT)/impl/dipper_fast_const.c \
           $(ROOT)/impl/dipper_ct.c $(ROOT)/impl/dipper_leaky.c
INC     := -I. -I$(ROOT)/common -I$(ROOT)/impl
LDLIBS  := -lm
ifeq ($(shell uname),Darwin)
MAPFLAG :=
else
MAPFLAG := -Wl,-Map,$$@.map
endif
ifeq ($(STATIC),1)
LDFLAGS += -static
endif
BUILD_INFO := "$(shell $(CC) --version | head -1 | tr ' ' '_') $(OPT)"

dudect_host: $(SRC) $(ROOT)/common/*.h $(ROOT)/impl/*.h platform.h
	$(CC) $(CFLAGS) $(EXTRA) $(INC) -DBUILD_INFO='$(BUILD_INFO)' -o $@ $(SRC) $(LDFLAGS) $(LDLIBS) $(MAPFLAG)
	nm -n $@ | grep -E ' (S|SI|SB2|SB2I|PT|PTI|P|PI|SB2C|SB2IC|PTC|PTIC)$$' > $@.tables.txt || true

clean:
	rm -f dudect_host dudect_host.map dudect_host.tables.txt
.PHONY: clean
