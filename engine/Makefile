
.PHONY: clean cleanall

SOURCES = message-buffer.c	\
	  smd.c			\
	  channel.c		\
	  event-capture.c	\
	  message-dispatcher.c	\
	  edsm-api.c

OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -fPIC

all: $(OBJECTS)

clean:
	@rm -fv $(OBJECTS)
cleanall:
	@rm -fv $(OBJECTS)
