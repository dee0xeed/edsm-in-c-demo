
.PHONY: clean cleanall

SOURCES = queue-array-test.c
OBJECTS = $(SOURCES:.c=.o)
PROGRAM = test
LDFLAGS = -ldl -lm

UTILS  =
UTILS += $(TOPDIR)/utils/list.o
UTILS += $(TOPDIR)/utils/queue.o
UTILS += $(TOPDIR)/utils/api.o
UTILS += $(TOPDIR)/utils/load-tool.o
UTILS += $(TOPDIR)/utils/mempool.o
UTILS += $(TOPDIR)/utils/logging.o

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(UTILS)
	$(CC) -rdynamic $(OBJECTS) $(UTILS) $(ENGINE) -o $@ $(LDFLAGS)

clean:
	@rm -fv $(OBJECTS)

cleanall:
	@rm -fv $(OBJECTS) $(PROGRAM)
