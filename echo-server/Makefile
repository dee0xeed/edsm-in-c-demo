
.PHONY: clean cleanall

SOURCES = $(wildcard state-machines/*.c)
SOURCES += main.c

OBJECTS = $(SOURCES:.c=.o)
PROGRAM = echod
LDFLAGS = -ldl -lsystemd

ENGINE = $(TOPDIR)/engine/*.o
UTILS  = $(TOPDIR)/utils/*.o

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(UTILS) $(ENGINE)
	$(CC) -rdynamic $(OBJECTS) $(UTILS) $(ENGINE) -o $@ $(LDFLAGS)

clean:
	@rm -fv $(OBJECTS)

cleanall:
	@rm -fv $(OBJECTS) $(PROGRAM)
