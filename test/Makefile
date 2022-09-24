
.PHONY: clean cleanall

SOURCES = main.c state-machines/test.c
OBJECTS = $(SOURCES:.c=.o)
PROGRAM = test
LDFLAGS = -ldl -lm

ENGINE  =
ENGINE += $(TOPDIR)/engine/message-buffer.o
ENGINE += $(TOPDIR)/engine/smd.o
ENGINE += $(TOPDIR)/engine/channel.o
ENGINE += $(TOPDIR)/engine/event-capture.o
ENGINE += $(TOPDIR)/engine/message-dispatcher.o
ENGINE += $(TOPDIR)/engine/edsm-api.o

UTILS  =
UTILS += $(TOPDIR)/utils/load-tool.o
UTILS += $(TOPDIR)/utils/strings.o
UTILS += $(TOPDIR)/utils/api.o
UTILS += $(TOPDIR)/utils/logging.o

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) $(UTILS) $(ENGINE)
	$(CC) -rdynamic $(OBJECTS) $(UTILS) $(ENGINE) -o $@ $(LDFLAGS)

clean:
	@rm -fv $(OBJECTS)

cleanall:
	@rm -fv $(OBJECTS) $(PROGRAM)
