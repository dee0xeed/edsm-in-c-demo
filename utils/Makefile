
.PHONY: clean cleanall

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -fPIC

all: $(OBJECTS)

clean:
	@rm -fv $(OBJECTS)
cleanall:
	@rm -fv $(OBJECTS)
