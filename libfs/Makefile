# Target library
lib := libfs.a
objs := disk.o fs.o
CC := gcc
CC_Lib := ar rcs
CFLAGS := -Wall -Werror

ifeq ($(D),1)	
	CFLAGS += -g
else
	CFLAGS += -O2
endif

ifneq ($(V),1)
	Q = @
endif

all: $(lib)

deps := $(patsubt %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(lib): $(objs)
	@echo "CC $@"
	$(Q)$(CC_Lib) -o $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "clean"
	$(Q)rm -f $(lib) $(objs) $(deps)


